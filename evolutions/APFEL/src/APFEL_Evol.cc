//Automatically generated by ./tools/AddEvolution.py on 2019-05-05

#include "APFEL_Evol.h"
//These might be useful
#include "xfitter_cpp_base.h" //for hf_errlog
#include "xfitter_pars.h"
#include "APFEL/APFEL.h"
#include <math.h>
#include "BasePdfDecomposition.h"
#include <algorithm>

// Global var to hold current pdfDecomposition
xfitter::BasePdfDecomposition *gPdfDecomp;

// Global photon or not PDF
bool gQCDevol;

// wraper for APFEL PDF function:
extern "C" void externalsetapfel1_(double &x, double &Q, double *xf)
{
  // xf is from -6 to +6 (or +7),
  for (int i = 0; i <= 12; i++)
  {
    xf[i] = 0.0;
  }
  if (x > 1.0) // it may happen.
    x = 1.0;

  const std::map<int, int> ip =
      {{-3, -3}, {-2, -2}, {-1, -1}, {0, 21}, {1, 1}, {2, 2}, {3, 3}};
  for (auto &i : ip)
  {
    xf[i.first + 6] = gPdfDecomp->xfxMap(x)[i.second];
  }
  if (!gQCDevol)
  {
    xf[13] = gPdfDecomp->xfxMap(x)[22];
  }
}

namespace xfitter
{
//for dynamic loading
extern "C" APFEL_Evol *create(const char *s) { return new APFEL_Evol(s); }

// helper to parse yaml sequences of uniform type
template <class T>
vector<T> getSeq(const YAML::Node node)
{
  if (!node.IsSequence())
  {
    std::cerr << "[ERROR]getSeq: node=\n"
              << node << std::endl;
    hf_errlog(190508150, "F: In APFEL evolution in function getSeq: wrong node type, expected sequence");
  }
  size_t len = node.size();
  vector<T> v(len);
  for (size_t i = 0; i < len; i++)
  {
    v[i] = node[i].as<T>();
  }
  return v;
}

void APFEL_Evol::atStart()
{
  // Initialize APFEL with the input parameters
  // Evolution order:
  const int PtOrder = OrderMap(XFITTER_PARS::getParamS("Order"));
  const double *Mz = XFITTER_PARS::getParamD("Mz");

  const double *mch = XFITTER_PARS::getParamD("mch");
  const double *mbt = XFITTER_PARS::getParamD("mbt");
  const double *mtp = XFITTER_PARS::getParamD("mtp");

  const double *alphas = XFITTER_PARS::getParamD("alphas");
  _Q0 = *XFITTER_PARS::getParamD("Q0");

  // Retrieve parameters needed to initialize DIS APFEL.
  const double *sin2thw = XFITTER_PARS::getParamD("sin2thW");
  const double *Vud = XFITTER_PARS::getParamD("Vud");
  const double *Vus = XFITTER_PARS::getParamD("Vus");
  const double *Vub = XFITTER_PARS::getParamD("Vub");
  const double *Vcd = XFITTER_PARS::getParamD("Vcd");
  const double *Vcs = XFITTER_PARS::getParamD("Vcs");
  const double *Vcb = XFITTER_PARS::getParamD("Vcb");
  const double *Vtd = XFITTER_PARS::getParamD("Vtd");
  const double *Vts = XFITTER_PARS::getParamD("Vts");
  const double *Vtb = XFITTER_PARS::getParamD("Vtb");
  const double *gf = XFITTER_PARS::getParamD("gf");
  const double *Mw = XFITTER_PARS::getParamD("Mw");

  // Read yaml steering
  _yAPFEL = XFITTER_PARS::getEvolutionNode(_name);
  const int iSplineOrder = _yAPFEL["SplineOrder"].as<int>();

  vector<double> xGrid = getSeq<double>(_yAPFEL["xGrid"]);
  vector<int> nxGrid = getSeq<int>(_yAPFEL["nxGrid"]);

  const size_t iNxGrids = nxGrid.size();

  if (nxGrid.size() != xGrid.size())
  {
    hf_errlog(2019050502, "S: APFEL evolution YAML setup error: nxGrid and xGrid sizes not the same");
  }

  vector<double> qLimits = getSeq<double>(_yAPFEL["qLimits"]);
  const string heavyQuarkMassScheme = _yAPFEL["heavyQuarkMassScheme"].as<string>();
  const string heavyQuarkMassRunning = _yAPFEL["heavyQuarkMassRunning"].as<string>();
  const string theoryType = _yAPFEL["theoryType"].as<string>();
  const string nllxResummation = _yAPFEL["nllxResummation"].as<string>();

  if (theoryType == "QCD")
  {
    _evolType = evolType::QCD;
  }
  else if (theoryType == "QUniD")
  {
    _evolType = evolType::QUniD;
  }
  else
  {
    hf_errlog(2019050601, "S: Unknown APFEL evolution theoryType, QCD or QUniD exected got " + theoryType);
  }


  // FONLL-specific settings
  const string scheme = "FONLL-" + _yAPFEL["FONLLVariant"].as<string>();

  if (PtOrder == 1)
  {
    //const string msg = "F: FONLL at LO not available. Use the ZM-VFNS instead.";
    //hf_errlog_(17120601,msg.c_str(), msg.size());
  }
  else if (PtOrder == 2 && scheme == "FONLL-C")
  {
    const string msg = "F: At NLO only the FONLL-A and FONLL-B schemes are possible";
    hf_errlog(17120602, msg);
  }
  else if (PtOrder == 3 && (scheme == "FONLL-A" || scheme == "FONLL-B"))
  {
    const string msg = "F: At NNLO only the FONLL-C scheme is possible";
    hf_errlog(17120603, msg);
  }
  else
  {
    APFEL::SetMassScheme(scheme);
  }



  APFEL::SetTheory(theoryType);
  // following 3 lines are copied from the fortran steering:
  APFEL::SetPDFEvolution("exactalpha"); // faster vs muF
  APFEL::SetFastEvolution(true);
  APFEL::SetAlphaEvolution("exact");

  // treat number of flavours
  int nflavour = XFITTER_PARS::gParametersI.at("NFlavour");
  APFEL::SetMaxFlavourPDFs(nflavour);
  APFEL::SetMaxFlavourAlpha(nflavour);
  //
  // this seems to be not needed (are two lines above sufficient?)
  // Another issue: is APFEL::SetFFNS(4) equivavelent to APFEL::SetMaxFlavourPDFs(4)
  // and APFEL::SetMaxFlavourAlpha(4) below the charm quark threshold?
  //
  //if(nflavour == 3 || nflavour == 4)
  //  APFEL::SetFFNS(nflavour);
  //else
  //  APFEL::SetVFNS();

  APFEL::SetAlphaQCDRef(*alphas, *Mz);
  APFEL::SetPerturbativeOrder(PtOrder - 1); //APFEL counts from 0

  // Set Parameters
  APFEL::SetZMass(*Mz);                 // make fittable at some point
  APFEL::SetWMass(*Mw);
  APFEL::SetSin2ThetaW(*sin2thw);
  APFEL::SetGFermi(*gf);
  APFEL::SetCKM(*Vud, *Vus, *Vub,
                *Vcd, *Vcs, *Vcb,
                *Vtd, *Vts, *Vtb);
  APFEL::EnableDynamicalScaleVariations(true);

  APFEL::SetQLimits(qLimits[0], qLimits[1]);
  // Setup x sub-grids:
  APFEL::SetNumberOfGrids(iNxGrids);
  for (size_t igrid = 0; igrid < iNxGrids; igrid++)
  {
    APFEL::SetGridParameters(igrid + 1, nxGrid[igrid], iSplineOrder, xGrid[igrid]);
  }
  APFEL::LockGrids(true);

  if (heavyQuarkMassScheme == "Pole")
  {
    APFEL::SetPoleMasses(*mch, *mbt, *mtp);
  }
  else
  {
    APFEL::SetMSbarMasses(*mch, *mbt, *mtp);
    if (heavyQuarkMassRunning == "On")
    {
      APFEL::EnableMassRunning(true);
    }
    else
    {
      APFEL::EnableMassRunning(false);
    }
  }
  // heavy-quark thresholds
  double kmc = _yAPFEL["kmc"] ? _yAPFEL["kmc"].as<double>() : 1.0;
  double kmb = _yAPFEL["kmb"] ? _yAPFEL["kmb"].as<double>() : 1.0;
  double kmt = _yAPFEL["kmt"] ? _yAPFEL["kmt"].as<double>() : 1.0;
  APFEL::SetMassMatchingScales(kmc, kmb, kmt);

  if (nllxResummation == "On")
  {
    APFEL::SetSmallxResummation(true, "NLL");
    APFEL::SetQLimits(1.6, 4550.0); // Hardwire for now.
  }

  // set the ratio muR / Q (default 1), muF / Q (default 1)
  double muRoverQ = _yAPFEL["muRoverQ"].as<double>();
  double muFoverQ = _yAPFEL["muFoverQ"].as<double>();
  APFEL::EnableDynamicalScaleVariations(false); // ??? somehow in the past this was needed if muRoverQ != 1, muFoverQ != 1
  APFEL::SetRenQRatio(muRoverQ);
  APFEL::SetFacQRatio(muFoverQ);

  // APFEL::InitializeAPFEL();
  // Initialize the APFEL DIS module
  APFEL::InitializeAPFEL_DIS();

  APFEL::SetPDFSet("external1");
  gPdfDecomp = XFITTER_PARS::getInputDecomposition(_yAPFEL);
  BaseEvolution::atStart();
}

void APFEL_Evol::atIteration()
{
  _Qlast = -1.;
  const double *Mz = XFITTER_PARS::getParamD("Mz");
  const double *alphas = XFITTER_PARS::getParamD("alphas");
  APFEL::SetAlphaQCDRef(*alphas, *Mz);
  gPdfDecomp = XFITTER_PARS::getInputDecomposition(_yAPFEL);
  BaseEvolution::atIteration();
}

void APFEL_Evol::atConfigurationChange()
{
  BaseEvolution::atConfigurationChange();
}

//
std::map<int, double> APFEL_Evol::xfxQmap(double x, double Q)
{
  double pdfs[14];
  xfxQarray(x, Q, pdfs);
  std::map<int, double> res;

  int npdfMax = (_evolType == evolType::QCD) ? 6 : 7;

  for (int ipdf = -6; ipdf <= npdfMax; ipdf++)
  {
    int ii = (ipdf == 0) ? 21 : ipdf;
    // photon PDF:
    if (ipdf == 7)
      ii = 22;
    res[ii] = pdfs[ipdf+6];
  }
  return res;
}

// define vs xfxQarray
double APFEL_Evol::xfxQ(int i, double x, double Q)
{
  double pdfs[14];
  xfxQarray(x, Q, pdfs);
  return pdfs[i + 6];
}

void APFEL_Evol::xfxQarray(double x, double Q, double *pdfs)
{
  gQCDevol = (_evolType == evolType::QCD);
  if (Q != _Qlast)
  {
    APFEL::EvolveAPFEL(_Q0, Q);
    _Qlast = Q;
  }
  if (_evolType == evolType::QCD)
  {
    APFEL::xPDFall(x, pdfs);
  }
  else if (_evolType == evolType::QUniD)
  {
    APFEL::xPDFallPhoton(x, pdfs);
  }
}

double APFEL_Evol::getAlphaS(double Q)
{
  return APFEL::AlphaQCD(Q);
}
} // namespace xfitter
