/*
   @file ReactionHathorMSR.cc
   @date 2020-06-16
   @author  AddReaction.py
   Created by  AddReaction.py on 2018-07-25, modified 2020-06-16
*/

#include "ReactionHathorMSR.h"
#include "HathorPdfxFitter.h"
#include "Hathor.h"
#include "cstring"
#include "xfitter_cpp.h"

// the class factories
extern "C" ReactionHathorMSR* create() {
    return new ReactionHathorMSR();
}

extern "C"
{
int rlxd_size(void);
void rlxd_get(int state[]);
void rlxd_reset(int state[]);
void rlxd_init(int level,int seed);
}

ReactionHathorMSR::ReactionHathorMSR() {
    _pdf = NULL;
    _rndStore = NULL;
}

ReactionHathorMSR::~ReactionHathorMSR() {
    if(_rndStore)
      delete[] _rndStore;
  
    // do NOT delete Hathor instances here, because:
    // (1) Hathor classes do not have vitual destructors which produces a warning
    // (2) Hathor classes do not have destructors at all
  
    //if(_pdf)
    //  delete _pdf;
    //for(auto item : _hathorArray)
    //  if(item.second)
    //    delete item.second;
}

void ReactionHathorMSR::initTerm(TermData *td) {

    ReactionTheory::initTerm(td);
    int dataSetID = td->id;
    _tdDS[dataSetID] = td;

    //Check if dataset with provided ID already exists
    if(_hathorArray.find(dataSetID) != _hathorArray.end()) {
        char str[256];
        sprintf(str, "F: dataset with the id = %d already exists", dataSetID);
        hf_errlog_(19060401, str, strlen(str));
    }

    //Read center-of-mass energy from dataset parameters (must be given)
    double sqrtS = 0.0;    //N.B. this will be retrieved from .dat file!
    if(td->hasParam("SqrtS")) sqrtS = *td->getParamD("SqrtS");
    if(sqrtS == 0.0) {
        char str[256];
        sprintf(str, "F: no SqrtS for dataset with id = %d", dataSetID);
        hf_errlog_(18081702, str, strlen(str));
    }
  
    //Read precision level. If unspecified, default to 2 -> Hathor::MEDIUM
    int precisionLevel = Hathor::MEDIUM;
    if(td->hasParam("precisionLevel")) {
        precisionLevel = td->getParamI("precisionLevel");
	}
    precisionLevel = std::pow(10, 2 + precisionLevel);
    // check that this setting is allowed
    // see in AbstractHathor.h:
    //   enum ACCURACY { LOW=1000, MEDIUM=10000, HIGH=100000 };
    // and
    // precisionLevel = 1 -> Hathor::LOW, 2 -> Hathor::MEDIUM, 3 -> Hathor::HIGH
    if(precisionLevel !=  Hathor::LOW    && 
       precisionLevel !=  Hathor::MEDIUM && 
       precisionLevel !=  Hathor::HIGH     )
    {
        char str[256];
        sprintf(str, "F: provided precision level = %d not supported by Hathor",
                precisionLevel);
        hf_errlog_(18081702, str, strlen(str));
    }

    // read ppbar from provided dataset parameters
    // if not specified assume it is false (pp collisions)
    int ppbar = false;
    if(td->hasParam("ppbar")) {
        ppbar = td->getParamI("ppbar");
        if(ppbar !=  0 && ppbar != 1) {
            char str[256];
            sprintf(str,"F: provided ppbar = %d not recognised (must be 0 or 1)", 
                    ppbar);
            hf_errlog_(17081103, str, strlen(str));
        }
    }

    //New Hathor object
    Hathor* hathor = new Hathor(*_pdf);
    hathor->sethc2(0.38937911e9);     //MCFM value

    //Collider type
    if (ppbar) hathor->setColliderType(Hathor::PPBAR);
    else       hathor->setColliderType(Hathor::PP);

    std::cout << "ReactionHathorMSR: PP/PPBAR parameter set to " << ppbar
              << std::endl;

    //Centre-of-mass energy
    hathor->setSqrtShad(sqrtS);
    std::cout << "ReactionHathorMSR: center of mass energy set to " << sqrtS
              << std::endl;

    //#[active light flavours], in case nfl!=5 also implemented in the future
    if(td->hasParam("HATHORNFL")) nfl = *td->getParamD("HATHORNFL");

    //Input top quark mass. May be recomputed in compute depending on scheme
    _mtop[dataSetID] = *td->getParamD("mtp");

    // scheme (perturbative order and pole/running mass treatment)
    std::string orderS = td->getParamS("Order");
    orderI = 0;           //Default to LO
    if(orderS == "NLO" ) orderI = 1;
    if(orderS == "NNLO") orderI = 2;
    mScheme = 0;    //Pole scheme by default
    Rscale_in = 0;  //Corresponds to pole scheme
    if(td->hasParam("MSCHEME")) mScheme = td->getParamI("MSCHEME");
    else cout << "ReactionHathorMSR: Could not find MSCHEME" << endl;
    if(mScheme == 1) Rscale_in = _mtop[dataSetID];    //MSbar
    else if(mScheme > 1 && td->hasParam("MSRSCALE"))  //MSRn & MSRp
        Rscale_in = *td->getParamD("MSRSCALE");
    else cout << "ReactionHathorMSR: Could not find MSRSCALE" << endl;
    convertMass = false;  //Flipped if MSBAR -> MSR mass conversion requested
    int convTmp = 0;
    if(td->hasParam("MSBAR2MSR")) convTmp = td->getParamI("MSBAR2MSR");
    if (convTmp != 0) convertMass = true;

    //Set alpha_S beta coef.s, needs orderI and nfl [#flavors]
    beta0  = 11. -  2.*nfl/3.;
    beta1 = orderI > 0 ? 102. - 38.*nfl/3. : 0.;
    beta2 = orderI > 1 ? 325./54.*nfl*nfl - 5033./18.*nfl + 2857./2. : 0.;
    bar0   = beta0/pow(4.*pi,2);
    bar1   = beta1/pow(4.*pi,4);
  
  hathor->setScheme(_scheme[dataSetID]);                              //TODO REMOVE -- obsolete for MSR?
  std::cout << "ReactionHathorMSR: Setting the scheme" << std::endl;  //TODO REMOVE -- obsolete for MSR?

    //Precision level
    hathor->setPrecision(precisionLevel);

    //Renormalization scale
    _mr[dataSetID] = _mtop[dataSetID];
    if(td->hasParam("muR")) _mr[dataSetID] *= *td->getParamD("muR");

    //Factorization scale
    _mf[dataSetID] = _mtop[dataSetID];
    if(td->hasParam("muF")) _mf[dataSetID] *= *td->getParamD("muF");

    std::cout << " Hathor will use:";
    std::cout << " mtop(input) = " << _mtop[dataSetID] << "[GeV] ";
    std::cout << " renorm. scale = " << _mr[dataSetID] << "[GeV] ";
    std::cout << " fact. scale = "   << _mf[dataSetID] << "[GeV]";
    std::cout << std::endl;
    if(mScheme == 0) std::cout << " Pole scheme chosen" << std::endl;
    if(mScheme == 1) std::cout << " MSbar scheme chosen" << std::endl;
    if(mScheme > 1 ) std::cout << " MSRSCALE = " << Rscale << std::endl;
    if(convertMass) {
        if(mScheme > 1) std::cout << " Converting MSbar mass to MSR" << std::endl;
        else {
  		  std::cout << "mScheme<2, no MSBAR->MSR conversion done" << std::endl;
  		  convertMass = false;
  	  }
    }
    std::cout << " #[active light flavors] = " << nfl << std::endl;
    std::cout << std::endl;

    //Done
    hathor->PrintOptions();
    _hathorArray[dataSetID] = hathor;
}

//Initialize at the start of the computation
void ReactionHathorMSR::atStart()
{
    // PDFs for Hathor
    _pdf = new HathorPdfxFitter(this);
  
    // random number generator
    rlxd_init(1, 1);
    int nRnd = rlxd_size();
    //std::cout << " Size of random number array = " << nRnd << "\n";
    _rndStore = new int[nRnd];
    rlxd_get(_rndStore);
}

// Decoupling coefficients
double ReactionHathorMSR::d1func(double Lmu) {
    return (4./3. + Lmu);
}
double ReactionHathorMSR::d2func(double Lmu) {
    return ( 307./32.
            + 2.*z2
            + 2./3.*z2*ln2
            - z3/6. 
            + 509./72.*Lmu
            + 47./24.*pow(Lmu,2) 
            - nfl*(71./144. + z2/3. + 13./36.*Lmu +pow(Lmu,2)/12.));
}

// Compute coefficients for transforming to arbitrary alpha_s(mu) via the eq.
//   as(m_MSbar)=as(mu)(1 +  as(mu_r)*(4*pi^2)*Lrbar*bar0
//                        + as^2(mu_r)*(4*pi^2)^2*( Lrbar*bar1+Lrbar^2*bar0^2))
vector<double> ReactionHathorMSR::asFactors(Hathor* XS, 
                                            double muOLD, double muNEW)
{
    vector<double> ret;   //n:th component will be the factor for as^(n+1)
    for (int n=0; n!=4; ++n) ret.push_back(1.);  //Init

    if (mScheme == 0) return ret;  //No factors needed in pole scheme

    double Lmu   = log(pow(muNEW/muOLD,2));
    double asOLD = XS->getAlphas(muOLD);
    double asNEW = XS->getAlphas(muNEW);
    
    ret[0] += asNEW*4*pi*Lmu*bar0;               //LO for c-s. is O(as^2)
    if (orderI != 0) {                           //O(as^3)
        ret[0] += pow((asNEW*4*pi),2)*( Lmu*bar1 + pow(Lmu*bar0,2));
        ret[1] += 8.*pi*asNEW*Lmu*bar0;
        if (orderI > 1) {                        //O(as^4)
            ret[1] +=  16.*pow(pi*asNEW*Lmu*bar0,2)
                     + 32.*pow(pi*asNEW,2)*( Lmu*bar1 + pow(Lmu*bar0,2) );
            ret[2]  = 1. + 12.*pi*asNEW*Lmu*bar0;
            //All additions to as4res would be O(as^5)
        }
    }
    for (unsigned int n=0; n!=ret.size(); ++n) ret[n] *= pow(asNEW/asOLD, n+1.);
    
    return ret;
}

//Integrand for MSbar evolution mt -> mu_m (=R) as in hep-ph/9703278 Eq. (1)
double mEvoInt_MSbar(Hathor* H, double R, int n) {
    return 2./R*pow(H->getAlphas(R)/pi,n+1);
}
//Integrand for MSR evolution
double mEvoInt_MSR(Hathor* H, double R, int n) {
    return pow(H->getAlphas(R)/pi,n+1);
}


// Evaluate the integral needed for R-evolution's integral from R1 to R0
// Param:    integrand  Pointer to the integrand function; par. Hathor obj, R, n
//           R0  Starting scale
//           R1  Scale to evolve to
//           n   Evaluate integral of n:th term in the RGE sol. series (start 0)
double ReactionHathorMSR::evoInt(Hathor* XS, 
                                 double (*integrand)(Hathor*,double,int),
                                 double R0, double R1, int n)
{
    if (n > orderI) return 0.;   //For consistency w/ PDF alpha_s loop order

    double Rmin     = min(R0,R1);
    double Rmax     = max(R0,R1);
    int    nbins    = 1000; //>1e3 converge (tested R=mt->1). 1e2 error O(0.001)
    double integral = 0.0;
    double R        = Rmin;
    double dR       = 0.;   //Step size determined later
    
    //Numerical integration w/ simple importance sampling
    double lobd = integrand(XS,R,n);
    double upbd = 0.0;
    double dRbasis = (Rmax-Rmin)/pow(nbins,4);      //No repeated evaluations
    for (int ibin=0; ibin != nbins; ++ibin) {
        dR = (pow(ibin+1,4)-pow(ibin,4))*dRbasis;
        R += dR;
        upbd = integrand(XS,R,n);
        integral += 0.5*(upbd+lobd)*dR;   //Simple trapezoid
        lobd = upbd;        
    }
    if (R0 > R1) integral *= -1.;

    return integral;
}


void ReactionHathorMSR::compute(TermData *td, valarray<double> &val, map<string, valarray<double> > &err)
{
    td->actualizeWrappers();
    _pdf->IsValid = true;
    int dataSetID = td->id;
    rlxd_reset(_rndStore);

    Hathor* hathor = _hathorArray.at(dataSetID);

    double mt_in = _mtop[dataSetID];     //Input mass
    double mt    =  mt_in;               //Changed later if need be //TODO def here or in class?
    double mur   = _mr[dataSetID];                                  //TODO def here or in class?
    double muf   = _mf[dataSetID];                                  //TODO def here or in class?

    Rscale = Rscale_in;    //TODO make sure Rscale_in  read in init, not Rscale
    mScheme = mScheme_in;  //TODO make sure mScheme_in read in init, not mScheme
    if      (mScheme == 0                 ) Rscale = 0.;  //Pole scheme
    else if (convertMass && Rscale > mt_in) mScheme = 1;  //Switch to MSBAR evo above MSBAR mt (if given)

    unsigned int scheme;
    double dmt=mt/100.;  //mt/100 proven OK. Too small lead to fluctuations
  
    double csFULL=0.;   //All contributions summed
    double   csLO=0.,  csLOp=0.,  csLOm=0.;
    double  csNLO=0., csNLOp=0., csNLOm=0.;
    double csNNLO=0.;
    double csup=0.,csdown=0.,cstmp=0.; //For PDF uncertainty calculations
    double errMSR=0., chiMSR=0.;
  
    //Decoupling coefficients 
    double LR = 0.;        //log(pow(mur/Rscale,2))=0 in general case
    double d1dec=0., d2dec=0., d3dec=0.;
    switch (mScheme) {
        case 0:       //Pole -- no coefficients to set
            break;
        case 1:       //MSbar
            d1dec = d1func(LR);
            d2dec = d2func(LR);
            d3dec = 190.59511171875 - 26.6551003125*nfl + 0.652690625*nfl*nfl;
            break;
        //MSR coefficients given in 1704.01580
        //N.B. 1) dNdec must be divided by 4^N to compensate for the different 
        //        convention of alphas/(4pi) vs alphas/pi.
        //     2) 1704.01580 doesn't give the log terms, which must be analogous
        //        to MSbar so that Rscale = mt yields MSbar result. 
        case 2:       //MSRn -- coef.s agree w/ MSbar when nfl=5 & nh=0
            d1dec = 4./3.;
            d2dec = 13.3398125 -  1.04136875*nfl;
            d3dec = 188.671875 - 26.67734375*nfl + 0.652690625*nfl*nfl;
            break;
        case 3:       //MSRp -- 1&2 coef.s agree w/ MSbar w/ nfl=5 & nh=1. 3 not.
          d1dec = d1func(LR);
          d2dec = d2func(LR);
          d3dec = 190.390625 - 26.65515625*nfl + 0.652690625*nfl*nfl;
          break;
        default:
            cout << "ERROR: Unknown mScheme. Cannot set d1dec, d2dec, d3dec."
                 << endl;
            return;          
    } 

    double aspi = hathor->getAlphas(mScheme==0 ? mur : Rscale)/pi;
    //Coefficients for generalizing cross-section to arbitrary alpha_s(mu_r)
    vector<double> asFac = asFactors(hathor, mScheme==0 ? mur : Rscale, mur);
    if (asFac.size()<4) {      //TODO make asFac only 3 relevant entries?
        cout << "ERROR in calculating as conversion factors" << endl;
        return;
    }
    double asLO   = asFac[1];  //TODO make asFac only 3 relevant entries?
    double asNLO  = asFac[2];  //TODO make asFac only 3 relevant entries?
    double asNNLO = asFac[3];  //TODO make asFac only 3 relevant entries?
  
    //Anomalous dimensions for mass evolution.
    //  MSR 1704.01580 (converted to HATHOR conventions) by default, changed 
    //  later if need be
    double gamR0 = d1dec;
    double gamR1 = d2dec - beta0*d1dec/2.;
    double gamR2 = d3dec - beta0*d2dec - beta1*d1dec/8.;

    //Evolve m_MSR from mt_MSbar(mt_MSbar) to scale R (or mu_m if MSbar evo)
    double mDelta=0.;
    double mMatched = mt_in;
    double asmt = XS.getAlphas(mt);
    double (*integrand)(Hathor*,double,int);    //Ptr to integrand in m RGE sol.
    integrand = mScheme==1 ? &mEvoInt_MSbar : &mEvoInt_MSR;
    double const Ncol=3., Ncol2=9., Ncol3=27., Ncol3=81.;  //SU(N=3) and powers
    if (mScheme != 0) {
        if (mScheme==1) {  //Use MSbar anom. dim. hep-ph/9703278 Eq. (4)->
            gam0=(Ncol2 - 1.)*3./4./Ncol/2.;
            gam1=(Ncol2 - 1.)/16./Ncol2*(-3./8. + 203.*Ncol2/24. - nfl*Ncol*5./6.);
            gam2=(Ncol2 - 1.)/64./Ncol3*(129./16.*(1-Ncol2) + 11413.*Ncol4/216.
                                     + nfl*(  Ncol*23./4. - Ncol3*1177./108.
                                            - Ncol*z3*6.  - Ncol3*z3*6. )
                                     - nfl*nfl*Ncol2*35./54.);
        } else if (convertMass && mScheme==2) { //MSRn
            //MSbar->MSRn needs 1704.01580 matching relation Eq. (5.8), MSRp not.
            //Here asmt must be alpha_s(mt_MSbar(mt_MSbar))
            mMatched *= (1. + pow(asmt/(4*pi),2)*1.65707
                            + (order>0 ? 1. : 0.)*pow(asmt/(4*pi),3)
                                                 *(110.05 + 1.424*nfl)
                            + (order>1 ? 1. : 0.)*pow(asmt/(4*pi),4)
                                                 *(344.-111.59*nfl+4.4*nfl*nfl) );      
        }
        mDelta = - gam0*evoInt(integrand,mMatched,R,0)   
                 - gam1*evoInt(integrand,mMatched,R,1)   
                 - gam2*evoInt(integrand,mMatched,R,2);
        if (mScheme == 1) mt *= exp(mDelta);      
        else              mt = mMatched + mDelta;
    }
    
    //Now that we have mt_MSbar(R) if need be, reset dec.coef. etc for MSbar evo
    if (mScheme == 1) {
        LR = log(pow(R/mt,2));    //ln((mu_m/mt(mu_m))^2), mu_m=R
        asmt = XS.getAlphas(mt);  //Update to as(mt_MSbar(R))
        d1dec = d1func(LR);
        d2dec = d2func(LR);
        /* TODO d3dec LR dependency unavailable. However, MSbar d3dec is
         *      unnecessary in this implementation.
         */
        //d3dec = 190.59511171875 - nfl*26.6551003125 + nfl*nfl*0.652690625;
    }

    //Set scheme. 
    //N.B. DO NOT USE HATHOR::MS_MASS HERE (or other scheme flags)! 
    //The cs is computed in the pole scheme and the result is converted into a 
    //running mass scheme in this program, independent of src code, w/ a stencil
    scheme = Hathor::LO;          //Start w/ LO contributions
    hathor->setScheme(scheme);

    // LO
    hathor->getXsection(mt, mScheme==0 ? mur : Rscale, muf);
    hathor->getResult(0,csLO,errMSR,chiMSR); 
    // LO derivatives
    if (mScheme!=0 && orderI > 0) {
        hathor->getXsection(mt+dmt,Rscale,muf);    
        hathor->getResult(0,csLOp,errMSR,chiMSR);
        hathor->getXsection(mt-dmt,Rscale,muf);    
        hathor->getResult(0,csLOm,errMSR,chiMSR);
    }

    // NLO
    if (orderI > 0) {
        scheme = Hathor::NLO;    //NLO contrib only not LO+NLO
        hathor->setScheme(scheme);
        hathor->getXsection(mt, mScheme==0 ? mur : Rscale, muf);
        hathor->getResult(0,csNLO,errMSR,chiMSR);
        // NLO derivatives
        if (mScheme!=0) {
            hathor->getXsection(mt+dmt,Rscale,muf);    
            hathor->getResult(0,csNLOp,errMSR,chiMSR);
            hathor->getXsection(mt-dmt,Rscale,muf);    
            hathor->getResult(0,csNLOm,errMSR,chiMSR);
        }
    }

    // NNLO
    if (orderI > 1) {
        scheme = Hathor::NNLO;    //NNLO contrib only
        hathor->setScheme(scheme);
        hathor->getXsection(mt, mScheme==0 ? mur : Rscale, muf);
        hathor->getResult(0,csNNLO,errMSR,chiMSR);
    }

    //Combine terms to get cross-section
    double NLOder=0., NNLOder=0.;
    double Rfac = mScheme==1 ? mt : R;                //Use mt_MSbar(R) or R?
    csFULL = csLO*asLO                                //Common LO
               + (orderI > 0 ? csNLO *asNLO  : 0.)    //Common NLO
               + (orderI > 1 ? csNNLO*asNNLO : 0.);   //Common NNLO
    if (mScheme!=0) {		   //Extra terms for running mass schemes
        if (orderI > 0) {
            NLOder = aspi*d1dec*Rfac/(2.*dmt)*(csLOp-csLOm);
            csFULL += asNLO*NLOder;
        }
        if (orderI > 1) {
            //N.B. csNLO terms include one factor of aspi on 2nd line
            NNLOder = pow(aspi,2)*d2dec*Rfac/(2.*dmt)*(csLOp -csLOm )                  
                     +       aspi*d1dec*Rfac/(2.*dmt)*(csNLOp-csNLOm)
                     +pow(aspi*d1dec*Rfac/dmt,2)/2.*(csLOp-2.*csLO+csLOm);
            csFULL += asNNLO*NNLOder;
        }
    }

    val[0] = csFULL;  //"Return value"

    //Write log file for Hathor results. TODO comment out when testing finished
    ifstream in("ReactionHathorMSR.log");
    string line;
    int linesRead = 0;
    while (getline(in, line)) ++linesRead;
    in.close();
    ofstream out("ReactionHathorMSR.log",ios::app);
    if (linesRead == 0) { //Write data column header if file is empty
        out << " Ord   outScheme      mt       R        alpS(mt)"
            << "       csFULL[0]     muf     mur" << endl;    
    }
    out << setfill(' ') << setw( 4) << orderI;
    out << setfill(' ') << setw(12) << mScheme;
    out << setfill(' ') << setw( 8) << mt;
    out << setfill(' ') << setw( 8) << Rscale;
    out << setfill(' ') << setw(16) << XS.getAlphas(mt);
    out << setfill(' ') << setw(16) << csFULL;
    out << setfill(' ') << setw( 8) << muf;    
    out << setfill(' ') << setw( 8) << mur;
    out << endl;

    out.close();
  
}

