 
/*
   @file ReactionAFB.cc
   @date 2018-07-16
   @author  AddReaction.py
   Created by  AddReaction.py on 2018-07-16
*/

#include "ReactionAFB.h"
#include "iostream"
#include <gsl/gsl_integration.h>

using namespace std;

// Declaration of static parameters
double ReactionAFB::PI;
double ReactionAFB::GeVtofb_param, ReactionAFB::alphaEM_param, ReactionAFB::stheta2W_param, ReactionAFB::MZ_param, ReactionAFB::GammaZ_param;
double ReactionAFB::energy_param, ReactionAFB::eta_cut_param, ReactionAFB::pT_cut_param, ReactionAFB::y_min_param, ReactionAFB::y_max_param;

double ReactionAFB::e_param, ReactionAFB::gsm_param, ReactionAFB::smangle_param;
double ReactionAFB::foton_Vu, ReactionAFB::foton_Au, ReactionAFB::foton_Vd, ReactionAFB::foton_Ad, ReactionAFB::foton_Vl, ReactionAFB::foton_Al, ReactionAFB::foton_Vnu, ReactionAFB::foton_Anu;
double ReactionAFB::Z_Vu, ReactionAFB::Z_Au, ReactionAFB::Z_Vd, ReactionAFB::Z_Ad, ReactionAFB::Z_Vl, ReactionAFB::Z_Al, ReactionAFB::Z_Vnu, ReactionAFB::Z_Anu;
double ReactionAFB::even_foton_up, ReactionAFB::even_foton_down, ReactionAFB::even_interf_up, ReactionAFB::even_interf_down, ReactionAFB::even_Z_up, ReactionAFB::even_Z_down;
double ReactionAFB::odd_foton_up, ReactionAFB::odd_foton_down, ReactionAFB::odd_interf_up, ReactionAFB::odd_interf_down, ReactionAFB::odd_Z_up, ReactionAFB::odd_Z_down;

double ReactionAFB::epsabs = 0;
double ReactionAFB::epsrel = 1e-2;

size_t ReactionAFB::calls;


//// Function returning the combination of propagators
double *ReactionAFB::propagators (double Minv)
{
    // Propagators squared and interference
    double foton_squared = 1.0/pow(Minv,4);
    double interference = 2.0*(-pow(Minv,2)*(pow(MZ_param,2)-pow(Minv,2)))/(pow(Minv,4)*((pow(pow(MZ_param,2)-pow(Minv,2),2))+pow(MZ_param,2)*pow(GammaZ_param,2)));
    double Z_squared = 1.0/(pow(pow(MZ_param,2)-pow(Minv,2),2)+pow(MZ_param,2)*pow(GammaZ_param,2));
    
    static double propagators[4];    
    propagators[0] = (even_foton_up * foton_squared) + (even_interf_up * interference) + (even_Z_up * Z_squared);
    propagators[1] = (odd_foton_up * foton_squared) + (odd_interf_up * interference) + (odd_Z_up * Z_squared);
    propagators[2] = (even_foton_down * foton_squared) + (even_interf_down * interference) + (even_Z_down * Z_squared);
    propagators[3] = (odd_foton_down * foton_squared) + (odd_interf_down * interference) + (odd_Z_down * Z_squared);
        
    return propagators;
}

////UUBAR EVEN FORWARD Matrix element
double ReactionAFB::uubarEF_funct (double yreduced, void * params) {
    
    // Pointer to access PDFs
    ReactionTheory* ptr = (ReactionTheory*) ((integration_params*)params)->ptr;    
    // Pass the invariant mass as parameter
    double Minv = ((integration_params*)params)-> Minv;
    
    // Partonic cross section parameters
    double Q = Minv;
    double z = pow(Minv,2)/pow(energy_param,2);
    double y = -(1.0/2.0)*log(z)*(yreduced);
    double x1 = sqrt(z)*exp(y);
    double x2 = sqrt(z)*exp(-y);
    double dsigma_temp = pow(Minv,2)/(96*PI);
    double dsigma = GeVtofb_param*dsigma_temp*(2*Minv/pow(energy_param,2))*(-(1.0/2.0)*log(z));
    
    // Partons PDFs
    std::valarray<double> pdfx1(13);
    std::valarray<double> pdfx2(13);
    (ptr->xfx(x1,Q,&pdfx1[0]));
    (ptr->xfx(x2,Q,&pdfx2[0]));
    double f1u = pdfx1[8] / x1;
    double f1c = pdfx1[10] / x1;
    double f2ubar = pdfx2[4] / x2;
    double f2cbar = pdfx2[2] / x2;

    // PDF combinations    
    double uubar_PDF = f1u*f2ubar + f1c*f2cbar;
    
    // Angular integration limits
    double qqbar_cos_theta_max = min(cos(2*atan(exp(-eta_cut_param-y))),sqrt(1-4*(pow(pT_cut_param,2)/pow(Minv,2))));
    double qqbar_cos_theta_min = 0;
 
    double angular_integration_EF = (qqbar_cos_theta_max-qqbar_cos_theta_min)+(1.0/3.0)*(pow(qqbar_cos_theta_max,3)-pow(qqbar_cos_theta_min,3));
    
    // Combination with angular integration (Forward - Backward for q-qbar)
    double dsigma_EF = dsigma*angular_integration_EF;

    // Covolution with PDFs (flipping direction for q-qbar & qbar-q)
    // U-UBAR
    double uubarEF = uubar_PDF*dsigma_EF;
  
    double *propagator = propagators (Minv);
    
    return uubarEF * propagator[0]; // Multiply the PDFs combination with the correct propagator.
}

////UUBAR EVEN FORWARD Integration in rapidity
double ReactionAFB::integration_uubarEF_y (double Minv, void * ptr) {
    
    // Pass the necessary parameters (pointer to the PDFs and Minv)
    integration_params integrationParams;
    integrationParams.Minv = Minv;
    integrationParams.ptr = (ReactionTheory*) ptr;
    
    double result, error;

    gsl_function F;
    F.function = &(ReactionAFB::uubarEF_funct);
    F.params = &integrationParams;
    
    double inf = y_min_param / log(energy_param/Minv);
    double sup;
    if (y_max_param == 0.0) {
        sup = 1;
    } else {
        sup = y_max_param / log(energy_param/Minv);
    }
    
    gsl_integration_qng (&F, inf, sup, epsabs, epsrel, &result, &error, &calls); 

    return 2*result;
}

////UUBAR EVEN FORWARD Integration in invariant mass
double ReactionAFB::integration_uubarEF (double Minv_inf, double Minv_sup, void* ptr) {
    
    double result, error;

    gsl_function F;
    F.function = &(ReactionAFB::integration_uubarEF_y);
    F.params = ptr;

    double inf = Minv_inf;
    double sup = Minv_sup;
    
    gsl_integration_qng (&F, inf, sup, epsabs, epsrel, &result, &error, &calls); 

    return result;
}

////UUBAR EVEN BACKWARD Matrix element
double ReactionAFB::uubarEB_funct (double yreduced, void * params) {
    
    // Pointer to access PDFs
    ReactionTheory* ptr = (ReactionTheory*) ((integration_params*)params)->ptr;    
    // Pass the invariant mass as parameter
    double Minv = ((integration_params*)params)-> Minv;
        
    // Partonic cross section parameters
    double Q = Minv;
    double z = pow(Minv,2)/pow(energy_param,2);
    double y = -(1.0/2.0)*log(z)*(yreduced);
    double x1 = sqrt(z)*exp(y);
    double x2 = sqrt(z)*exp(-y);
    double dsigma_temp = pow(Minv,2)/(96*PI);
    double dsigma = GeVtofb_param*dsigma_temp*(2*Minv/pow(energy_param,2))*(-(1.0/2.0)*log(z));
    
    // Partons PDFs
    std::valarray<double> pdfx1(13);
    std::valarray<double> pdfx2(13);
    (ptr->xfx(x1,Q,&pdfx1[0]));
    (ptr->xfx(x2,Q,&pdfx2[0]));
    double f1u = pdfx1[8] / x1;
    double f1c = pdfx1[10] / x1;
    double f2ubar = pdfx2[4] / x2;
    double f2cbar = pdfx2[2] / x2;

    // PDF combinations    
    double uubar_PDF = f1u*f2ubar + f1c*f2cbar;
    
    // Angular integration limits
    double qbarq_cos_theta_max = 0;
    double qbarq_cos_theta_min = max(cos(PI - 2*atan(exp(-eta_cut_param-y))),-sqrt(1-4*(pow(pT_cut_param,2)/pow(Minv,2))));   
 
    double angular_integration_EB = (qbarq_cos_theta_max-qbarq_cos_theta_min)+(1.0/3.0)*(pow(qbarq_cos_theta_max,3)-pow(qbarq_cos_theta_min,3));
    
    // Combination with angular integration (Forward - Backward for q-qbar)
    double dsigma_EB = dsigma*angular_integration_EB;

    // Covolution with PDFs (flipping direction for q-qbar & qbar-q)
    // U-UBAR
    double uubarEB = uubar_PDF*dsigma_EB;
  
    double *propagator = propagators (Minv);
    return uubarEB * propagator[0];
}

////UUBAR EVEN BACKWARD Integration in rapidity
double ReactionAFB::integration_uubarEB_y (double Minv, void * ptr) {
    
    // Pass the necessary parameters (pointer to the PDFs and Minv)
    integration_params integrationParams;
    integrationParams.Minv = Minv;
    integrationParams.ptr = (ReactionTheory*) ptr;
    
    double result, error;

    gsl_function F;
    F.function = &(ReactionAFB::uubarEB_funct);
    F.params = &integrationParams;
    
    double inf = y_min_param / log(energy_param/Minv);
    double sup;
    if (y_max_param == 0.0) {
        sup = 1;
    } else {
        sup = y_max_param / log(energy_param/Minv);
    }
    
    gsl_integration_qng (&F, inf, sup, epsabs, epsrel, &result, &error, &calls); 

    return 2*result;
}

////UUBAR EVEN BACKWARD Integration in invariant mass
double ReactionAFB::integration_uubarEB (double Minv_inf, double Minv_sup, void* ptr) {
    
    double result, error;

    gsl_function F;
    F.function = &(ReactionAFB::integration_uubarEB_y);
    F.params = ptr;
    
    double inf = Minv_inf;
    double sup = Minv_sup;
    
    gsl_integration_qng (&F, inf, sup, epsabs, epsrel, &result, &error, &calls); 

    return result;
}

////UUBAR ODD FORWARD Matrix element
double ReactionAFB::uubarOF_funct (double yreduced, void * params) {
    
    // Pointer to access PDFs
    ReactionTheory* ptr = (ReactionTheory*) ((integration_params*)params)->ptr;    
    // Pass the invariant mass as parameter
    double Minv = ((integration_params*)params)-> Minv;
        
    // Partonic cross section parameters
    double Q = Minv;
    double z = pow(Minv,2)/pow(energy_param,2);
    double y = -(1.0/2.0)*log(z)*(yreduced);
    double x1 = sqrt(z)*exp(y);
    double x2 = sqrt(z)*exp(-y);
    double dsigma_temp = pow(Minv,2)/(96*PI);
    double dsigma = GeVtofb_param*dsigma_temp*(2*Minv/pow(energy_param,2))*(-(1.0/2.0)*log(z));
    
    // Partons PDFs
    std::valarray<double> pdfx1(13);
    std::valarray<double> pdfx2(13);
    (ptr->xfx(x1,Q,&pdfx1[0]));
    (ptr->xfx(x2,Q,&pdfx2[0]));
    double f1u = pdfx1[8] / x1;
    double f1c = pdfx1[10] / x1;
    double f2ubar = pdfx2[4] / x2;
    double f2cbar = pdfx2[2] / x2;

    // PDF combinations    
    double uubar_PDF = f1u*f2ubar + f1c*f2cbar;
    
    // Angular integration limits
    double qqbar_cos_theta_max = min(cos(2*atan(exp(-eta_cut_param-y))),sqrt(1-4*(pow(pT_cut_param,2)/pow(Minv,2))));
    double qqbar_cos_theta_min = 0;  
 
    double angular_integration_OF = pow(qqbar_cos_theta_max,2) - pow(qqbar_cos_theta_min,2);
    
    // Combination with angular integration (Forward - Backward for q-qbar)
    double dsigma_OF = dsigma*angular_integration_OF;

    // Covolution with PDFs (flipping direction for q-qbar & qbar-q)
    // U-UBAR
    double uubarOF = uubar_PDF*dsigma_OF;
  
    double *propagator = propagators (Minv);
    
    return uubarOF * propagator[1];
}

////UUBAR ODD FORWARD Integration in rapidity
double ReactionAFB::integration_uubarOF_y (double Minv, void * ptr) {
    
    // Pass the necessary parameters (pointer to the PDFs and Minv)
    integration_params integrationParams;
    integrationParams.Minv = Minv;
    integrationParams.ptr = (ReactionTheory*) ptr;
    
    double result, error;

    gsl_function F;
    F.function = &(ReactionAFB::uubarOF_funct);
    F.params = &integrationParams;
    
    double inf = y_min_param / log(energy_param/Minv);
    double sup;
    if (y_max_param == 0.0) {
        sup = 1;
    } else {
        sup = y_max_param / log(energy_param/Minv);
    }
    
    gsl_integration_qng (&F, inf, sup, epsabs, epsrel, &result, &error, &calls); 

    return 2*result;
}

////UUBAR ODD FORWARD Integration in invariant mass
double ReactionAFB::integration_uubarOF (double Minv_inf, double Minv_sup, void* ptr) {
    
    double result, error;

    gsl_function F;
    F.function = &(ReactionAFB::integration_uubarOF_y);
    F.params = ptr;
    
    double inf = Minv_inf;
    double sup = Minv_sup;
    
    gsl_integration_qng (&F, inf, sup, epsabs, epsrel, &result, &error, &calls); 

    return result;
}

////UUBAR ODD BACKWARD Matrix element
double ReactionAFB::uubarOB_funct (double yreduced, void * params) {
    
    // Pointer to access PDFs
    ReactionTheory* ptr = (ReactionTheory*) ((integration_params*)params)->ptr;    
    // Pass the invariant mass as parameter
    double Minv = ((integration_params*)params)-> Minv;
        
    // Partonic cross section parameters
    double Q = Minv;
    double z = pow(Minv,2)/pow(energy_param,2);
    double y = -(1.0/2.0)*log(z)*(yreduced);
    double x1 = sqrt(z)*exp(y);
    double x2 = sqrt(z)*exp(-y);
    double dsigma_temp = pow(Minv,2)/(96*PI);
    double dsigma = GeVtofb_param*dsigma_temp*(2*Minv/pow(energy_param,2))*(-(1.0/2.0)*log(z));
    
    // Partons PDFs
    std::valarray<double> pdfx1(13);
    std::valarray<double> pdfx2(13);
    (ptr->xfx(x1,Q,&pdfx1[0]));
    (ptr->xfx(x2,Q,&pdfx2[0]));
    double f1u = pdfx1[8] / x1;
    double f1c = pdfx1[10] / x1;
    double f2ubar = pdfx2[4] / x2;
    double f2cbar = pdfx2[2] / x2;

    // PDF combinations    
    double uubar_PDF = f1u*f2ubar + f1c*f2cbar;
    
    // Angular integration limits
    double qbarq_cos_theta_max = 0;
    double qbarq_cos_theta_min = max(cos(PI - 2*atan(exp(-eta_cut_param-y))),-sqrt(1-4*(pow(pT_cut_param,2)/pow(Minv,2)))); 
 
    double angular_integration_OB = pow(qbarq_cos_theta_max,2) - pow(qbarq_cos_theta_min,2);
    
    // Combination with angular integration (Forward - Backward for q-qbar)
    double dsigma_OB = dsigma*angular_integration_OB;

    // Covolution with PDFs (flipping direction for q-qbar & qbar-q)
    // U-UBAR
    double uubarOB = uubar_PDF*dsigma_OB;
  
    double *propagator = propagators (Minv);
    
    return uubarOB * propagator[1];
}

////UUBAR ODD BACKWARD Integration in rapidity
double ReactionAFB::integration_uubarOB_y (double Minv, void * ptr) {
    
    // Pass the necessary parameters (pointer to the PDFs and Minv)
    integration_params integrationParams;
    integrationParams.Minv = Minv;
    integrationParams.ptr = (ReactionTheory*) ptr;
    
    double result, error;

    gsl_function F;
    F.function = &(ReactionAFB::uubarOB_funct);
    F.params = &integrationParams;
    
    double inf = y_min_param / log(energy_param/Minv);
    double sup;
    if (y_max_param == 0.0) {
        sup = 1;
    } else {
        sup = y_max_param / log(energy_param/Minv);
    }
    
    gsl_integration_qng (&F, inf, sup, epsabs, epsrel, &result, &error, &calls); 

    return 2*result;
}

////UUBAR ODD BACKWARD Integration in invariant mass
double ReactionAFB::integration_uubarOB (double Minv_inf, double Minv_sup, void* ptr) {
    
    double result, error;

    gsl_function F;
    F.function = &(ReactionAFB::integration_uubarOB_y);
    F.params = ptr;
    
    double inf = Minv_inf;
    double sup = Minv_sup;
    
    gsl_integration_qng (&F, inf, sup, epsabs, epsrel, &result, &error, &calls); 

    return result;
}

////UBARU EVEN FORWARD Matrix element
double ReactionAFB::ubaruEF_funct (double yreduced, void * params) {
    
    // Pointer to access PDFs
    ReactionTheory* ptr = (ReactionTheory*) ((integration_params*)params)->ptr;    
    // Pass the invariant mass as parameter
    double Minv = ((integration_params*)params)-> Minv;
        
    // Partonic cross section parameters
    double Q = Minv;
    double z = pow(Minv,2)/pow(energy_param,2);
    double y = -(1.0/2.0)*log(z)*(yreduced);
    double x1 = sqrt(z)*exp(y);
    double x2 = sqrt(z)*exp(-y);
    double dsigma_temp = pow(Minv,2)/(96*PI);
    double dsigma = GeVtofb_param*dsigma_temp*(2*Minv/pow(energy_param,2))*(-(1.0/2.0)*log(z));
    
    // Partons PDFs
    std::valarray<double> pdfx1(13);
    std::valarray<double> pdfx2(13);
    (ptr->xfx(x1,Q,&pdfx1[0]));
    (ptr->xfx(x2,Q,&pdfx2[0]));
    double f1ubar = pdfx1[4] / x1;
    double f1cbar = pdfx1[2] / x1;
    double f2u = pdfx2[8] / x2;
    double f2c = pdfx2[10] / x2;

    // PDF combinations    
    double ubaru_PDF = f1ubar*f2u + f1cbar*f2c;
    
    // Angular integration limits
    double qbarq_cos_theta_max = 0;
    double qbarq_cos_theta_min = max(cos(PI - 2*atan(exp(-eta_cut_param-y))),-sqrt(1-4*(pow(pT_cut_param,2)/pow(Minv,2))));   
 
    double angular_integration_EB = (qbarq_cos_theta_max-qbarq_cos_theta_min)+(1.0/3.0)*(pow(qbarq_cos_theta_max,3)-pow(qbarq_cos_theta_min,3));
    
    // Combination with angular integration (Forward - Backward for q-qbar)
    double dsigma_EB = dsigma*angular_integration_EB;

    // Covolution with PDFs (flipping direction for q-qbar & qbar-q)
    // UBAR-U
    double ubaruEF = ubaru_PDF*dsigma_EB;
  
    double *propagator = propagators (Minv);
    
    return ubaruEF * propagator[0];
}

////UBARU EVEN FORWARD Integration in rapidity
double ReactionAFB::integration_ubaruEF_y (double Minv, void * ptr) {
    
    // Pass the necessary parameters (pointer to the PDFs and Minv)
    integration_params integrationParams;
    integrationParams.Minv = Minv;
    integrationParams.ptr = (ReactionTheory*) ptr;
    
    double result, error;

    gsl_function F;
    F.function = &(ReactionAFB::ubaruEF_funct);
    F.params = &integrationParams;
    
    double inf = y_min_param / log(energy_param/Minv);
    double sup;
    if (y_max_param == 0.0) {
        sup = 1;
    } else {
        sup = y_max_param / log(energy_param/Minv);
    }
    
    gsl_integration_qng (&F, inf, sup, epsabs, epsrel, &result, &error, &calls); 

    return 2*result;
}

////UBARU EVEN FORWARD Integration in invariant mass
double ReactionAFB::integration_ubaruEF (double Minv_inf, double Minv_sup, void* ptr) {
    
    double result, error;

    gsl_function F;
    F.function = &(ReactionAFB::integration_ubaruEF_y);
    F.params = ptr;
    
    double inf = Minv_inf;
    double sup = Minv_sup;
    
    gsl_integration_qng (&F, inf, sup, epsabs, epsrel, &result, &error, &calls); 

    return result;
}

////UBARU EVEN BACKWARD Matrix element
double ReactionAFB::ubaruEB_funct (double yreduced, void * params) {
    
    // Pointer to access PDFs
    ReactionTheory* ptr = (ReactionTheory*) ((integration_params*)params)->ptr;    
    // Pass the invariant mass as parameter
    double Minv = ((integration_params*)params)-> Minv;
        
    // Partonic cross section parameters
    double Q = Minv;
    double z = pow(Minv,2)/pow(energy_param,2);
    double y = -(1.0/2.0)*log(z)*(yreduced);
    double x1 = sqrt(z)*exp(y);
    double x2 = sqrt(z)*exp(-y);
    double dsigma_temp = pow(Minv,2)/(96*PI);
    double dsigma = GeVtofb_param*dsigma_temp*(2*Minv/pow(energy_param,2))*(-(1.0/2.0)*log(z));
    
    // Partons PDFs
    std::valarray<double> pdfx1(13);
    std::valarray<double> pdfx2(13);
    (ptr->xfx(x1,Q,&pdfx1[0]));
    (ptr->xfx(x2,Q,&pdfx2[0]));
    double f1ubar = pdfx1[4] / x1;
    double f1cbar = pdfx1[2] / x1;
    double f2u = pdfx2[8] / x2;
    double f2c = pdfx2[10] / x2;

    // PDF combinations    
    double ubaru_PDF = f1ubar*f2u + f1cbar*f2c;
    
    // Angular integration limits
    double qqbar_cos_theta_max = min(cos(2*atan(exp(-eta_cut_param-y))),sqrt(1-4*(pow(pT_cut_param,2)/pow(Minv,2))));
    double qqbar_cos_theta_min = 0;  
 
    double angular_integration_EF = (qqbar_cos_theta_max-qqbar_cos_theta_min)+(1.0/3.0)*(pow(qqbar_cos_theta_max,3)-pow(qqbar_cos_theta_min,3));
    
    // Combination with angular integration (Forward - Backward for q-qbar)
    double dsigma_EF = dsigma*angular_integration_EF;

    // Covolution with PDFs (flipping direction for q-qbar & qbar-q)
    // UBAR-U
    double ubaruEB = ubaru_PDF*dsigma_EF;
  
    double *propagator = propagators (Minv);
    
    return ubaruEB * propagator[0];
}

////UBARU EVEN BACKWARD Integration in rapidity
double ReactionAFB::integration_ubaruEB_y (double Minv, void * ptr) {
    
    // Pass the necessary parameters (pointer to the PDFs and Minv)
    integration_params integrationParams;
    integrationParams.Minv = Minv;
    integrationParams.ptr = (ReactionTheory*) ptr;
    
    double result, error;

    gsl_function F;
    F.function = &(ReactionAFB::ubaruEB_funct);
    F.params = &integrationParams;
    
    double inf = y_min_param / log(energy_param/Minv);
    double sup;
    if (y_max_param == 0.0) {
        sup = 1;
    } else {
        sup = y_max_param / log(energy_param/Minv);
    }
    
    gsl_integration_qng (&F, inf, sup, epsabs, epsrel, &result, &error, &calls); 

    return 2*result;
}

////UBARU EVEN BACKWARD Integration in invariant mass
double ReactionAFB::integration_ubaruEB (double Minv_inf, double Minv_sup, void* ptr) {
    
    double result, error;

    gsl_function F;
    F.function = &(ReactionAFB::integration_ubaruEB_y);
    F.params = ptr;
    
    double inf = Minv_inf;
    double sup = Minv_sup;
    
    gsl_integration_qng (&F, inf, sup, epsabs, epsrel, &result, &error, &calls); 

    return result;
}

////UBARU ODD FORWARD Matrix element
double ReactionAFB::ubaruOF_funct (double yreduced, void * params) {
    
    // Pointer to access PDFs
    ReactionTheory* ptr = (ReactionTheory*) ((integration_params*)params)->ptr;    
    // Pass the invariant mass as parameter
    double Minv = ((integration_params*)params)-> Minv;
        
    // Partonic cross section parameters
    double Q = Minv;
    double z = pow(Minv,2)/pow(energy_param,2);
    double y = -(1.0/2.0)*log(z)*(yreduced);
    double x1 = sqrt(z)*exp(y);
    double x2 = sqrt(z)*exp(-y);
    double dsigma_temp = pow(Minv,2)/(96*PI);
    double dsigma = GeVtofb_param*dsigma_temp*(2*Minv/pow(energy_param,2))*(-(1.0/2.0)*log(z));
    
    // Partons PDFs
    std::valarray<double> pdfx1(13);
    std::valarray<double> pdfx2(13);
    (ptr->xfx(x1,Q,&pdfx1[0]));
    (ptr->xfx(x2,Q,&pdfx2[0]));
    double f1ubar = pdfx1[4] / x1;
    double f1cbar = pdfx1[2] / x1;
    double f2u = pdfx2[8] / x2;
    double f2c = pdfx2[10] / x2;

    // PDF combinations    
    double ubaru_PDF = f1ubar*f2u + f1cbar*f2c;
    
    // Angular integration limits
    double qbarq_cos_theta_max = 0;
    double qbarq_cos_theta_min = max(cos(PI - 2*atan(exp(-eta_cut_param-y))),-sqrt(1-4*(pow(pT_cut_param,2)/pow(Minv,2)))); 
 
    double angular_integration_OB = pow(qbarq_cos_theta_max,2) - pow(qbarq_cos_theta_min,2);
    
    // Combination with angular integration (Forward - Backward for q-qbar)
    double dsigma_OB = dsigma*angular_integration_OB;

    // Covolution with PDFs (flipping direction for q-qbar & qbar-q)
    // UBAR-U
    double ubaruOF = ubaru_PDF*dsigma_OB;
  
    double *propagator = propagators (Minv);
    
    return ubaruOF * propagator[1];
}

////UBARU ODD FORWARD Integration in rapidity
double ReactionAFB::integration_ubaruOF_y (double Minv, void * ptr) {
    
    // Pass the necessary parameters (pointer to the PDFs and Minv)
    integration_params integrationParams;
    integrationParams.Minv = Minv;
    integrationParams.ptr = (ReactionTheory*) ptr;
    
    double result, error;

    gsl_function F;
    F.function = &(ReactionAFB::ubaruOF_funct);
    F.params = &integrationParams;
    
    double inf = y_min_param / log(energy_param/Minv);
    double sup;
    if (y_max_param == 0.0) {
        sup = 1;
    } else {
        sup = y_max_param / log(energy_param/Minv);
    }
    
    gsl_integration_qng (&F, inf, sup, epsabs, epsrel, &result, &error, &calls); 

    return 2*result;
}

////UBARU ODD FORWARD Integration in invariant mass
double ReactionAFB::integration_ubaruOF (double Minv_inf, double Minv_sup, void* ptr) {
    
    double result, error;

    gsl_function F;
    F.function = &(ReactionAFB::integration_ubaruOF_y);
    F.params = ptr;
    
    double inf = Minv_inf;
    double sup = Minv_sup;
    
    gsl_integration_qng (&F, inf, sup, epsabs, epsrel, &result, &error, &calls); 

    return result;
}

////UBARU ODD BACKWARD Matrix element
double ReactionAFB::ubaruOB_funct (double yreduced, void * params) {
    
    // Pointer to access PDFs
    ReactionTheory* ptr = (ReactionTheory*) ((integration_params*)params)->ptr;    
    // Pass the invariant mass as parameter
    double Minv = ((integration_params*)params)-> Minv;
        
    // Partonic cross section parameters
    double Q = Minv;
    double z = pow(Minv,2)/pow(energy_param,2);
    double y = -(1.0/2.0)*log(z)*(yreduced);
    double x1 = sqrt(z)*exp(y);
    double x2 = sqrt(z)*exp(-y);
    double dsigma_temp = pow(Minv,2)/(96*PI);
    double dsigma = GeVtofb_param*dsigma_temp*(2*Minv/pow(energy_param,2))*(-(1.0/2.0)*log(z));
    
    // Partons PDFs
    std::valarray<double> pdfx1(13);
    std::valarray<double> pdfx2(13);
    (ptr->xfx(x1,Q,&pdfx1[0]));
    (ptr->xfx(x2,Q,&pdfx2[0]));
    double f1ubar = pdfx1[4] / x1;
    double f1cbar = pdfx1[2] / x1;
    double f2u = pdfx2[8] / x2;
    double f2c = pdfx2[10] / x2;

    // PDF combinations    
    double ubaru_PDF = f1ubar*f2u + f1cbar*f2c;
    
    // Angular integration limits
    double qqbar_cos_theta_max = min(cos(2*atan(exp(-eta_cut_param-y))),sqrt(1-4*(pow(pT_cut_param,2)/pow(Minv,2))));
    double qqbar_cos_theta_min = 0; 
 
    double angular_integration_OF = pow(qqbar_cos_theta_max,2) - pow(qqbar_cos_theta_min,2);
    
    // Combination with angular integration (Forward - Backward for q-qbar)
    double dsigma_OF = dsigma*angular_integration_OF;

    // Covolution with PDFs (flipping direction for q-qbar & qbar-q)
    // UBAR-U
    double ubaruOB = ubaru_PDF*dsigma_OF;
  
    double *propagator = propagators (Minv);
    
    return ubaruOB * propagator[1];
}

////UBARU ODD BACKWARD Integration in rapidity
double ReactionAFB::integration_ubaruOB_y (double Minv, void * ptr) {
    
    // Pass the necessary parameters (pointer to the PDFs and Minv)
    integration_params integrationParams;
    integrationParams.Minv = Minv;
    integrationParams.ptr = (ReactionTheory*) ptr;
    
    double result, error;

    gsl_function F;
    F.function = &(ReactionAFB::ubaruOB_funct);
    F.params = &integrationParams;
    
    double inf = y_min_param / log(energy_param/Minv);
    double sup;
    if (y_max_param == 0.0) {
        sup = 1;
    } else {
        sup = y_max_param / log(energy_param/Minv);
    }
    
    gsl_integration_qng (&F, inf, sup, epsabs, epsrel, &result, &error, &calls); 

    return 2*result;
}

////UBARU ODD BACKWARD Integration in invariant mass
double ReactionAFB::integration_ubaruOB (double Minv_inf, double Minv_sup, void* ptr) {
    
    double result, error;

    gsl_function F;
    F.function = &(ReactionAFB::integration_ubaruOB_y);
    F.params = ptr;
    
    double inf = Minv_inf;
    double sup = Minv_sup;
    
    gsl_integration_qng (&F, inf, sup, epsabs, epsrel, &result, &error, &calls); 

    return result;
}

////DDBAR EVEN FORWARD Matrix element
double ReactionAFB::ddbarEF_funct (double yreduced, void * params) {
    
    // Pointer to access PDFs
    ReactionTheory* ptr = (ReactionTheory*) ((integration_params*)params)->ptr;    
    // Pass the invariant mass as parameter
    double Minv = ((integration_params*)params)-> Minv;
        
    // Partonic cross section parameters
    double Q = Minv;
    double z = pow(Minv,2)/pow(energy_param,2);
    double y = -(1.0/2.0)*log(z)*(yreduced);
    double x1 = sqrt(z)*exp(y);
    double x2 = sqrt(z)*exp(-y);
    double dsigma_temp = pow(Minv,2)/(96*PI);
    double dsigma = GeVtofb_param*dsigma_temp*(2*Minv/pow(energy_param,2))*(-(1.0/2.0)*log(z));
    
    // Partons PDFs
    std::valarray<double> pdfx1(13);
    std::valarray<double> pdfx2(13);
    (ptr->xfx(x1,Q,&pdfx1[0]));
    (ptr->xfx(x2,Q,&pdfx2[0]));
    double f1d = pdfx1[7] / x1;
    double f1s = pdfx1[9] / x1;
    double f1b = pdfx1[11] / x1;
    double f2dbar = pdfx2[5] / x2;
    double f2sbar = pdfx2[3] / x2;
    double f2bbar = pdfx2[1] / x2;

    // PDF combinations    
    double ddbar_PDF = f1d*f2dbar + f1s*f2sbar + f1b*f2bbar;
    
    // Angular integration limits
    double qqbar_cos_theta_max = min(cos(2*atan(exp(-eta_cut_param-y))),sqrt(1-4*(pow(pT_cut_param,2)/pow(Minv,2))));
    double qqbar_cos_theta_min = 0;
 
    double angular_integration_EF = (qqbar_cos_theta_max-qqbar_cos_theta_min)+(1.0/3.0)*(pow(qqbar_cos_theta_max,3)-pow(qqbar_cos_theta_min,3));
    
    // Combination with angular integration (Forward - Backward for q-qbar)
    double dsigma_EF = dsigma*angular_integration_EF;

    // Covolution with PDFs (flipping direction for q-qbar & qbar-q)
    // D-DBAR
    double ddbarEF = ddbar_PDF*dsigma_EF;
  
    double *propagator = propagators (Minv);
    
    return ddbarEF * propagator[2];
}

////DDBAR EVEN FORWARD Integration in rapidity
double ReactionAFB::integration_ddbarEF_y (double Minv, void * ptr) {
    
    // Pass the necessary parameters (pointer to the PDFs and Minv)
    integration_params integrationParams;
    integrationParams.Minv = Minv;
    integrationParams.ptr = (ReactionTheory*) ptr;
    
    double result, error;

    gsl_function F;
    F.function = &(ReactionAFB::ddbarEF_funct);
    F.params = &integrationParams;
    
    double inf = y_min_param / log(energy_param/Minv);
    double sup;
    if (y_max_param == 0.0) {
        sup = 1;
    } else {
        sup = y_max_param / log(energy_param/Minv);
    }
    
    gsl_integration_qng (&F, inf, sup, epsabs, epsrel, &result, &error, &calls); 

    return 2*result;
}

////DDBAR EVEN FORWARD Integration in invariant mass
double ReactionAFB::integration_ddbarEF (double Minv_inf, double Minv_sup, void* ptr) {
    
    double result, error;

    gsl_function F;
    F.function = &(ReactionAFB::integration_ddbarEF_y);
    F.params = ptr;
    
    double inf = Minv_inf;
    double sup = Minv_sup;
    
    gsl_integration_qng (&F, inf, sup, epsabs, epsrel, &result, &error, &calls); 

    return result;
}

////DDBAR EVEN BACKWARD Matrix element
double ReactionAFB::ddbarEB_funct (double yreduced, void * params) {
    
    // Pointer to access PDFs
    ReactionTheory* ptr = (ReactionTheory*) ((integration_params*)params)->ptr;    
    // Pass the invariant mass as parameter
    double Minv = ((integration_params*)params)-> Minv;
        
    // Partonic cross section parameters
    double Q = Minv;
    double z = pow(Minv,2)/pow(energy_param,2);
    double y = -(1.0/2.0)*log(z)*(yreduced);
    double x1 = sqrt(z)*exp(y);
    double x2 = sqrt(z)*exp(-y);
    double dsigma_temp = pow(Minv,2)/(96*PI);
    double dsigma = GeVtofb_param*dsigma_temp*(2*Minv/pow(energy_param,2))*(-(1.0/2.0)*log(z));
    
    // Partons PDFs
    std::valarray<double> pdfx1(13);
    std::valarray<double> pdfx2(13);
    (ptr->xfx(x1,Q,&pdfx1[0]));
    (ptr->xfx(x2,Q,&pdfx2[0]));
    double f1d = pdfx1[7] / x1;
    double f1s = pdfx1[9] / x1;
    double f1b = pdfx1[11] / x1;
    double f2dbar = pdfx2[5] / x2;
    double f2sbar = pdfx2[3] / x2;
    double f2bbar = pdfx2[1] / x2;

    // PDF combinations    
    double ddbar_PDF = f1d*f2dbar + f1s*f2sbar + f1b*f2bbar;
    
    // Angular integration limits
    double qbarq_cos_theta_max = 0;
    double qbarq_cos_theta_min = max(cos(PI - 2*atan(exp(-eta_cut_param-y))),-sqrt(1-4*(pow(pT_cut_param,2)/pow(Minv,2))));   
 
    double angular_integration_EB = (qbarq_cos_theta_max-qbarq_cos_theta_min)+(1.0/3.0)*(pow(qbarq_cos_theta_max,3)-pow(qbarq_cos_theta_min,3));
    
    // Combination with angular integration (Forward - Backward for q-qbar)
    double dsigma_EB = dsigma*angular_integration_EB;

    // Covolution with PDFs (flipping direction for q-qbar & qbar-q)
    // D-DBAR
    double ddbarEB = ddbar_PDF*dsigma_EB;
  
    double *propagator = propagators (Minv);
    
    return ddbarEB * propagator[2];
}

////DDBAR EVEN BACKWARD Integration in rapidity
double ReactionAFB::integration_ddbarEB_y (double Minv, void * ptr) {
    
    // Pass the necessary parameters (pointer to the PDFs and Minv)
    integration_params integrationParams;
    integrationParams.Minv = Minv;
    integrationParams.ptr = (ReactionTheory*) ptr;
    
    double result, error;

    gsl_function F;
    F.function = &(ReactionAFB::ddbarEB_funct);
    F.params = &integrationParams;
    
    double inf = y_min_param / log(energy_param/Minv);
    double sup;
    if (y_max_param == 0.0) {
        sup = 1;
    } else {
        sup = y_max_param / log(energy_param/Minv);
    }
    
    gsl_integration_qng (&F, inf, sup, epsabs, epsrel, &result, &error, &calls); 

    return 2*result;
}

////DDBAR EVEN BACKWARD Integration in invariant mass
double ReactionAFB::integration_ddbarEB (double Minv_inf, double Minv_sup, void* ptr) {
    
    double result, error;

    gsl_function F;
    F.function = &(ReactionAFB::integration_ddbarEB_y);
    F.params = ptr;
    
    double inf = Minv_inf;
    double sup = Minv_sup;
    
    gsl_integration_qng (&F, inf, sup, epsabs, epsrel, &result, &error, &calls); 

    return result;
}

////DDBAR ODD FORWARD Matrix element
double ReactionAFB::ddbarOF_funct (double yreduced, void * params) {
    
    // Pointer to access PDFs
    ReactionTheory* ptr = (ReactionTheory*) ((integration_params*)params)->ptr;    
    // Pass the invariant mass as parameter
    double Minv = ((integration_params*)params)-> Minv;
        
    // Partonic cross section parameters
    double Q = Minv;
    double z = pow(Minv,2)/pow(energy_param,2);
    double y = -(1.0/2.0)*log(z)*(yreduced);
    double x1 = sqrt(z)*exp(y);
    double x2 = sqrt(z)*exp(-y);
    double dsigma_temp = pow(Minv,2)/(96*PI);
    double dsigma = GeVtofb_param*dsigma_temp*(2*Minv/pow(energy_param,2))*(-(1.0/2.0)*log(z));
    
    // Partons PDFs
    std::valarray<double> pdfx1(13);
    std::valarray<double> pdfx2(13);
    (ptr->xfx(x1,Q,&pdfx1[0]));
    (ptr->xfx(x2,Q,&pdfx2[0]));
    double f1d = pdfx1[7] / x1;
    double f1s = pdfx1[9] / x1;
    double f1b = pdfx1[11] / x1;
    double f2dbar = pdfx2[5] / x2;
    double f2sbar = pdfx2[3] / x2;
    double f2bbar = pdfx2[1] / x2;

    // PDF combinations    
    double ddbar_PDF = f1d*f2dbar + f1s*f2sbar + f1b*f2bbar;
    
    // Angular integration limits
    double qqbar_cos_theta_max = min(cos(2*atan(exp(-eta_cut_param-y))),sqrt(1-4*(pow(pT_cut_param,2)/pow(Minv,2))));
    double qqbar_cos_theta_min = 0;  
 
    double angular_integration_OF = pow(qqbar_cos_theta_max,2) - pow(qqbar_cos_theta_min,2);
    
    // Combination with angular integration (Forward - Backward for q-qbar)
    double dsigma_OF = dsigma*angular_integration_OF;

    // Covolution with PDFs (flipping direction for q-qbar & qbar-q)
    // D-DBAR
    double ddbarOF = ddbar_PDF*dsigma_OF;
  
    double *propagator = propagators (Minv);
    
    return ddbarOF * propagator[3];
}

////DDBAR ODD FORWARD Integration in rapidity
double ReactionAFB::integration_ddbarOF_y (double Minv, void * ptr) {
    
    // Pass the necessary parameters (pointer to the PDFs and Minv)
    integration_params integrationParams;
    integrationParams.Minv = Minv;
    integrationParams.ptr = (ReactionTheory*) ptr;
    
    double result, error;

    gsl_function F;
    F.function = &(ReactionAFB::ddbarOF_funct);
    F.params = &integrationParams;
    
    double inf = y_min_param / log(energy_param/Minv);
    double sup;
    if (y_max_param == 0.0) {
        sup = 1;
    } else {
        sup = y_max_param / log(energy_param/Minv);
    }
    
    gsl_integration_qng (&F, inf, sup, epsabs, epsrel, &result, &error, &calls); 

    return 2*result;
}

////DDBAR ODD FORWARD Integration in invariant mass
double ReactionAFB::integration_ddbarOF (double Minv_inf, double Minv_sup, void* ptr) {
    
    double result, error;

    gsl_function F;
    F.function = &(ReactionAFB::integration_ddbarOF_y);
    F.params = ptr;
    
    double inf = Minv_inf;
    double sup = Minv_sup;
    
    gsl_integration_qng (&F, inf, sup, epsabs, epsrel, &result, &error, &calls); 

    return result;
}

////DDBAR ODD BACKWARD Matrix element
double ReactionAFB::ddbarOB_funct (double yreduced, void * params) {
    
    // Pointer to access PDFs
    ReactionTheory* ptr = (ReactionTheory*) ((integration_params*)params)->ptr;    
    // Pass the invariant mass as parameter
    double Minv = ((integration_params*)params)-> Minv;
        
    // Partonic cross section parameters
    double Q = Minv;
    double z = pow(Minv,2)/pow(energy_param,2);
    double y = -(1.0/2.0)*log(z)*(yreduced);
    double x1 = sqrt(z)*exp(y);
    double x2 = sqrt(z)*exp(-y);
    double dsigma_temp = pow(Minv,2)/(96*PI);
    double dsigma = GeVtofb_param*dsigma_temp*(2*Minv/pow(energy_param,2))*(-(1.0/2.0)*log(z));
    
    // Partons PDFs
    std::valarray<double> pdfx1(13);
    std::valarray<double> pdfx2(13);
    (ptr->xfx(x1,Q,&pdfx1[0]));
    (ptr->xfx(x2,Q,&pdfx2[0]));
    double f1d = pdfx1[7] / x1;
    double f1s = pdfx1[9] / x1;
    double f1b = pdfx1[11] / x1;
    double f2dbar = pdfx2[5] / x2;
    double f2sbar = pdfx2[3] / x2;
    double f2bbar = pdfx2[1] / x2;

    // PDF combinations    
    double ddbar_PDF = f1d*f2dbar + f1s*f2sbar + f1b*f2bbar;
    
    // Angular integration limits
    double qbarq_cos_theta_max = 0;
    double qbarq_cos_theta_min = max(cos(PI - 2*atan(exp(-eta_cut_param-y))),-sqrt(1-4*(pow(pT_cut_param,2)/pow(Minv,2)))); 
 
    double angular_integration_OB = pow(qbarq_cos_theta_max,2) - pow(qbarq_cos_theta_min,2);
    
    // Combination with angular integration (Forward - Backward for q-qbar)
    double dsigma_OB = dsigma*angular_integration_OB;

    // Covolution with PDFs (flipping direction for q-qbar & qbar-q)
    // D-DBAR
    double ddbarOB = ddbar_PDF*dsigma_OB;
  
    double *propagator = propagators (Minv);
    
    return ddbarOB * propagator[3];
}

////DDBAR ODD BACKWARD Integration in rapidity
double ReactionAFB::integration_ddbarOB_y (double Minv, void * ptr) {
    
    // Pass the necessary parameters (pointer to the PDFs and Minv)
    integration_params integrationParams;
    integrationParams.Minv = Minv;
    integrationParams.ptr = (ReactionTheory*) ptr;
    
    double result, error;

    gsl_function F;
    F.function = &(ReactionAFB::ddbarOB_funct);
    F.params = &integrationParams;
    
    double inf = y_min_param / log(energy_param/Minv);
    double sup;
    if (y_max_param == 0.0) {
        sup = 1;
    } else {
        sup = y_max_param / log(energy_param/Minv);
    }
    
    gsl_integration_qng (&F, inf, sup, epsabs, epsrel, &result, &error, &calls); 

    return 2*result;
}

////DDBAR ODD BACKWARD Integration in invariant mass
double ReactionAFB::integration_ddbarOB (double Minv_inf, double Minv_sup, void* ptr) {
    
    double result, error;

    gsl_function F;
    F.function = &(ReactionAFB::integration_ddbarOB_y);
    F.params = ptr;
    
    double inf = Minv_inf;
    double sup = Minv_sup;
    
    gsl_integration_qng (&F, inf, sup, epsabs, epsrel, &result, &error, &calls); 

    return result;
}

////DBARD EVEN FORWARD Matrix element
double ReactionAFB::dbardEF_funct (double yreduced, void * params) {
    
    // Pointer to access PDFs
    ReactionTheory* ptr = (ReactionTheory*) ((integration_params*)params)->ptr;    
    // Pass the invariant mass as parameter
    double Minv = ((integration_params*)params)-> Minv;
        
    // Partonic cross section parameters
    double Q = Minv;
    double z = pow(Minv,2)/pow(energy_param,2);
    double y = -(1.0/2.0)*log(z)*(yreduced);
    double x1 = sqrt(z)*exp(y);
    double x2 = sqrt(z)*exp(-y);
    double dsigma_temp = pow(Minv,2)/(96*PI);
    double dsigma = GeVtofb_param*dsigma_temp*(2*Minv/pow(energy_param,2))*(-(1.0/2.0)*log(z));
    
    // Partons PDFs
    std::valarray<double> pdfx1(13);
    std::valarray<double> pdfx2(13);
    (ptr->xfx(x1,Q,&pdfx1[0]));
    (ptr->xfx(x2,Q,&pdfx2[0]));
    double f1dbar = pdfx1[5] / x1;
    double f1sbar = pdfx1[3] / x1;
    double f1bbar = pdfx1[1] / x1;
    double f2d = pdfx2[7] / x2;
    double f2s = pdfx2[9] / x2;
    double f2b = pdfx2[11] / x2;

    // PDF combinations    
    double dbard_PDF = f1dbar*f2d + f1sbar*f2s + f1bbar*f2b;
    
    // Angular integration limits
    double qbarq_cos_theta_max = 0;
    double qbarq_cos_theta_min = max(cos(PI - 2*atan(exp(-eta_cut_param-y))),-sqrt(1-4*(pow(pT_cut_param,2)/pow(Minv,2))));   
 
    double angular_integration_EB = (qbarq_cos_theta_max-qbarq_cos_theta_min)+(1.0/3.0)*(pow(qbarq_cos_theta_max,3)-pow(qbarq_cos_theta_min,3));
    
    // Combination with angular integration (Forward - Backward for q-qbar)
    double dsigma_EB = dsigma*angular_integration_EB;

    // Covolution with PDFs (flipping direction for q-qbar & qbar-q)
    // DBAR-D
    double dbardEF = dbard_PDF*dsigma_EB;
  
    double *propagator = propagators (Minv);
    
    return dbardEF * propagator[2];
}

////DBARD EVEN FORWARD Integration in rapidity
double ReactionAFB::integration_dbardEF_y (double Minv, void * ptr) {
    
    // Pass the necessary parameters (pointer to the PDFs and Minv)
    integration_params integrationParams;
    integrationParams.Minv = Minv;
    integrationParams.ptr = (ReactionTheory*) ptr;
    
    double result, error;

    gsl_function F;
    F.function = &(ReactionAFB::dbardEF_funct);
    F.params = &integrationParams;
    
    double inf = y_min_param / log(energy_param/Minv);
    double sup;
    if (y_max_param == 0.0) {
        sup = 1;
    } else {
        sup = y_max_param / log(energy_param/Minv);
    }
    
    gsl_integration_qng (&F, inf, sup, epsabs, epsrel, &result, &error, &calls); 

    return 2*result;
}

////DBARD EVEN FORWARD Integration in invariant mass
double ReactionAFB::integration_dbardEF (double Minv_inf, double Minv_sup, void* ptr) {
    
    double result, error;

    gsl_function F;
    F.function = &(ReactionAFB::integration_dbardEF_y);
    F.params = ptr;
    
    double inf = Minv_inf;
    double sup = Minv_sup;
    
    gsl_integration_qng (&F, inf, sup, epsabs, epsrel, &result, &error, &calls); 

    return result;
}

////DBARD EVEN BACKWARD Matrix element
double ReactionAFB::dbardEB_funct (double yreduced, void * params) {
    
    // Pointer to access PDFs
    ReactionTheory* ptr = (ReactionTheory*) ((integration_params*)params)->ptr;    
    // Pass the invariant mass as parameter
    double Minv = ((integration_params*)params)-> Minv;
        
    // Partonic cross section parameters
    double Q = Minv;
    double z = pow(Minv,2)/pow(energy_param,2);
    double y = -(1.0/2.0)*log(z)*(yreduced);
    double x1 = sqrt(z)*exp(y);
    double x2 = sqrt(z)*exp(-y);
    double dsigma_temp = pow(Minv,2)/(96*PI);
    double dsigma = GeVtofb_param*dsigma_temp*(2*Minv/pow(energy_param,2))*(-(1.0/2.0)*log(z));
    
    // Partons PDFs
    std::valarray<double> pdfx1(13);
    std::valarray<double> pdfx2(13);
    (ptr->xfx(x1,Q,&pdfx1[0]));
    (ptr->xfx(x2,Q,&pdfx2[0]));
    double f1dbar = pdfx1[5] / x1;
    double f1sbar = pdfx1[3] / x1;
    double f1bbar = pdfx1[1] / x1;
    double f2d = pdfx2[7] / x2;
    double f2s = pdfx2[9] / x2;
    double f2b = pdfx2[11] / x2;

    // PDF combinations    
    double dbard_PDF = f1dbar*f2d + f1sbar*f2s + f1bbar*f2b;
    
    // Angular integration limits
    double qqbar_cos_theta_max = min(cos(2*atan(exp(-eta_cut_param-y))),sqrt(1-4*(pow(pT_cut_param,2)/pow(Minv,2))));
    double qqbar_cos_theta_min = 0;  
 
    double angular_integration_EF = (qqbar_cos_theta_max-qqbar_cos_theta_min)+(1.0/3.0)*(pow(qqbar_cos_theta_max,3)-pow(qqbar_cos_theta_min,3));
    
    // Combination with angular integration (Forward - Backward for q-qbar)
    double dsigma_EF = dsigma*angular_integration_EF;

    // Covolution with PDFs (flipping direction for q-qbar & qbar-q)
    // DBAR-D
    double dbardEB = dbard_PDF*dsigma_EF;
  
    double *propagator = propagators (Minv);
    
    return dbardEB * propagator[2];
}

////DBARD EVEN BACKWARD Integration in rapidity
double ReactionAFB::integration_dbardEB_y (double Minv, void * ptr) {
    
    // Pass the necessary parameters (pointer to the PDFs and Minv)
    integration_params integrationParams;
    integrationParams.Minv = Minv;
    integrationParams.ptr = (ReactionTheory*) ptr;
    
    double result, error;

    gsl_function F;
    F.function = &(ReactionAFB::dbardEB_funct);
    F.params = &integrationParams;
    
    double inf = y_min_param / log(energy_param/Minv);
    double sup;
    if (y_max_param == 0.0) {
        sup = 1;
    } else {
        sup = y_max_param / log(energy_param/Minv);
    }
    
    gsl_integration_qng (&F, inf, sup, epsabs, epsrel, &result, &error, &calls); 

    return 2*result;
}

////DBARD EVEN BACKWARD Integration in invariant mass
double ReactionAFB::integration_dbardEB (double Minv_inf, double Minv_sup, void* ptr) {
    
    double result, error;

    gsl_function F;
    F.function = &(ReactionAFB::integration_dbardEB_y);
    F.params = ptr;
    
    double inf = Minv_inf;
    double sup = Minv_sup;
    
    gsl_integration_qng (&F, inf, sup, epsabs, epsrel, &result, &error, &calls); 

    return result;
}

////DBARD ODD FORWARD Matrix element
double ReactionAFB::dbardOF_funct (double yreduced, void * params) {
    
    // Pointer to access PDFs
    ReactionTheory* ptr = (ReactionTheory*) ((integration_params*)params)->ptr;    
    // Pass the invariant mass as parameter
    double Minv = ((integration_params*)params)-> Minv;
        
    // Partonic cross section parameters
    double Q = Minv;
    double z = pow(Minv,2)/pow(energy_param,2);
    double y = -(1.0/2.0)*log(z)*(yreduced);
    double x1 = sqrt(z)*exp(y);
    double x2 = sqrt(z)*exp(-y);
    double dsigma_temp = pow(Minv,2)/(96*PI);
    double dsigma = GeVtofb_param*dsigma_temp*(2*Minv/pow(energy_param,2))*(-(1.0/2.0)*log(z));
    
    // Partons PDFs
    std::valarray<double> pdfx1(13);
    std::valarray<double> pdfx2(13);
    (ptr->xfx(x1,Q,&pdfx1[0]));
    (ptr->xfx(x2,Q,&pdfx2[0]));
    double f1dbar = pdfx1[5] / x1;
    double f1sbar = pdfx1[3] / x1;
    double f1bbar = pdfx1[1] / x1;
    double f2d = pdfx2[7] / x2;
    double f2s = pdfx2[9] / x2;
    double f2b = pdfx2[11] / x2;

    // PDF combinations    
    double dbard_PDF = f1dbar*f2d + f1sbar*f2s + f1bbar*f2b;
    
    // Angular integration limits
    double qbarq_cos_theta_max = 0;
    double qbarq_cos_theta_min = max(cos(PI - 2*atan(exp(-eta_cut_param-y))),-sqrt(1-4*(pow(pT_cut_param,2)/pow(Minv,2)))); 
 
    double angular_integration_OB = pow(qbarq_cos_theta_max,2) - pow(qbarq_cos_theta_min,2);
    
    // Combination with angular integration (Forward - Backward for q-qbar)
    double dsigma_OB = dsigma*angular_integration_OB;

    // Covolution with PDFs (flipping direction for q-qbar & qbar-q)
    // DBAR-D
    double dbardOF = dbard_PDF*dsigma_OB;
  
    double *propagator = propagators (Minv);
    
    return dbardOF * propagator[3];
}

////DBARD ODD FORWARD Integration in rapidity
double ReactionAFB::integration_dbardOF_y (double Minv, void * ptr) {
    
    // Pass the necessary parameters (pointer to the PDFs and Minv)
    integration_params integrationParams;
    integrationParams.Minv = Minv;
    integrationParams.ptr = (ReactionTheory*) ptr;
    
    double result, error;

    gsl_function F;
    F.function = &(ReactionAFB::dbardOF_funct);
    F.params = &integrationParams;
    
    double inf = y_min_param / log(energy_param/Minv);
    double sup;
    if (y_max_param == 0.0) {
        sup = 1;
    } else {
        sup = y_max_param / log(energy_param/Minv);
    }
    
    gsl_integration_qng (&F, inf, sup, epsabs, epsrel, &result, &error, &calls); 

    return 2*result;
}

////DBARD ODD FORWARD Integration in invariant mass
double ReactionAFB::integration_dbardOF (double Minv_inf, double Minv_sup, void* ptr) {
    
    double result, error;

    gsl_function F;
    F.function = &(ReactionAFB::integration_dbardOF_y);
    F.params = ptr;
    
    double inf = Minv_inf;
    double sup = Minv_sup;
    
    gsl_integration_qng (&F, inf, sup, epsabs, epsrel, &result, &error, &calls); 

    return result;
}

////DBARD ODD BACKWARD Matrix element
double ReactionAFB::dbardOB_funct (double yreduced, void * params) {
    
    // Pointer to access PDFs
    ReactionTheory* ptr = (ReactionTheory*) ((integration_params*)params)->ptr;    
    // Pass the invariant mass as parameter
    double Minv = ((integration_params*)params)-> Minv;
        
    // Partonic cross section parameters
    double Q = Minv;
    double z = pow(Minv,2)/pow(energy_param,2);
    double y = -(1.0/2.0)*log(z)*(yreduced);
    double x1 = sqrt(z)*exp(y);
    double x2 = sqrt(z)*exp(-y);
    double dsigma_temp = pow(Minv,2)/(96*PI);
    double dsigma = GeVtofb_param*dsigma_temp*(2*Minv/pow(energy_param,2))*(-(1.0/2.0)*log(z));
    
    // Partons PDFs
    std::valarray<double> pdfx1(13);
    std::valarray<double> pdfx2(13);
    (ptr->xfx(x1,Q,&pdfx1[0]));
    (ptr->xfx(x2,Q,&pdfx2[0]));
    double f1dbar = pdfx1[5] / x1;
    double f1sbar = pdfx1[3] / x1;
    double f1bbar = pdfx1[1] / x1;
    double f2d = pdfx2[7] / x2;
    double f2s = pdfx2[9] / x2;
    double f2b = pdfx2[11] / x2;

    // PDF combinations    
    double dbard_PDF = f1dbar*f2d + f1sbar*f2s + f1bbar*f2b;
    
    // Angular integration limits
    double qqbar_cos_theta_max = min(cos(2*atan(exp(-eta_cut_param-y))),sqrt(1-4*(pow(pT_cut_param,2)/pow(Minv,2))));
    double qqbar_cos_theta_min = 0; 
 
    double angular_integration_OF = pow(qqbar_cos_theta_max,2) - pow(qqbar_cos_theta_min,2);
    
    // Combination with angular integration (Forward - Backward for q-qbar)
    double dsigma_OF = dsigma*angular_integration_OF;

    // Covolution with PDFs (flipping direction for q-qbar & qbar-q)
    // DBAR-D
    double dbardOB = dbard_PDF*dsigma_OF;
  
    double *propagator = propagators (Minv);
    
    return dbardOB * propagator[3];
}

////DBARD ODD BACKWARD Integration in rapidity
double ReactionAFB::integration_dbardOB_y (double Minv, void * ptr) {
    
    // Pass the necessary parameters (pointer to the PDFs and Minv)
    integration_params integrationParams;
    integrationParams.Minv = Minv;
    integrationParams.ptr = (ReactionTheory*) ptr;
    
    double result, error;

    gsl_function F;
    F.function = &(ReactionAFB::dbardOB_funct);
    F.params = &integrationParams;
    
    double inf = y_min_param / log(energy_param/Minv);
    double sup;
    if (y_max_param == 0.0) {
        sup = 1;
    } else {
        sup = y_max_param / log(energy_param/Minv);
    }
    
    gsl_integration_qng (&F, inf, sup, epsabs, epsrel, &result, &error, &calls); 

    return 2*result;
}

////DBARD ODD BACKWARD Integration in invariant mass
double ReactionAFB::integration_dbardOB (double Minv_inf, double Minv_sup, void* ptr) {
    
    double result, error;

    gsl_function F;
    F.function = &(ReactionAFB::integration_dbardOB_y);
    F.params = ptr;
    
    double inf = Minv_inf;
    double sup = Minv_sup;
    
    gsl_integration_qng (&F, inf, sup, epsabs, epsrel, &result, &error, &calls); 

    return result;
}

double ReactionAFB::AFB (double Minv_inf, double Minv_sup)
{
    double uubarEF = integration_uubarEF (Minv_inf, Minv_sup, this);
    double uubarEB = integration_uubarEB (Minv_inf, Minv_sup, this);
    double uubarOF = integration_uubarOF (Minv_inf, Minv_sup, this);
    double uubarOB = integration_uubarOB (Minv_inf, Minv_sup, this);
    
    double ubaruEF = integration_ubaruEF (Minv_inf, Minv_sup, this);
    double ubaruEB = integration_ubaruEB (Minv_inf, Minv_sup, this);
    double ubaruOF = integration_ubaruOF (Minv_inf, Minv_sup, this);
    double ubaruOB = integration_ubaruOB (Minv_inf, Minv_sup, this);
    
    double ddbarEF = integration_ddbarEF (Minv_inf, Minv_sup, this);
    double ddbarEB = integration_ddbarEB (Minv_inf, Minv_sup, this);
    double ddbarOF = integration_ddbarOF (Minv_inf, Minv_sup, this);
    double ddbarOB = integration_ddbarOB (Minv_inf, Minv_sup, this);
        
    double dbardEF = integration_dbardEF (Minv_inf, Minv_sup, this); 
    double dbardEB = integration_dbardEB (Minv_inf, Minv_sup, this);
    double dbardOF = integration_dbardOF (Minv_inf, Minv_sup, this);
    double dbardOB = integration_dbardOB (Minv_inf, Minv_sup, this);
    
    // Reconstructed Forward and Backward
    double Forward = uubarEF+ubaruEF+ddbarEF+dbardEF+uubarOF+ubaruOF+ddbarOF+dbardOF;
    double Backward = uubarEB+ubaruEB+ddbarEB+dbardEB+uubarOB+ubaruOB+ddbarOB+dbardOB;
    
    // Reconstructed AFB
    double AFB = (Forward - Backward) / (Forward + Backward);
    
    return AFB;
}

// the class factories
extern "C" ReactionAFB* create() {
  return new ReactionAFB();
}

// Initialize at the start of the computation
int ReactionAFB::initAtStart(const string &s)
{    
    // Parameters from "/reactions/AFB/yaml/parameters.yaml"
    // Check energy parameter:
    std::cout << checkParam("energy") << std::endl;
    if ( ! checkParam("energy") ) {
        std::cout << "\n\n FATAL ERROR: collider energy (energy) is not defined !!! \n\n" <<std::endl;
        return 1;
    }
    // Check eta parameter:
    std::cout << checkParam("eta_cut") << std::endl;
    if ( ! checkParam("eta_cut") ) {
        std::cout << "\n\n FATAL ERROR: lepton pseudorapidity cut (eta_cut_param) is not defined !!! \n\n" <<std::endl;
        return 1;
    }
    // Check pT parameter:
    std::cout << checkParam("pT_cut") << std::endl;
    if ( ! checkParam("eta_cut") ) {
        std::cout << "\n\n FATAL ERROR: lepton transverse momentum cut (pT_cut_param) is not defined !!! \n\n" <<std::endl;
        return 1;
    }
    // Check rapidity lower cut parameter:
    std::cout << checkParam("y_min") << std::endl;
    if ( ! checkParam("y_min") ) {
        std::cout << "\n\n FATAL ERROR: di-lepton rapidity lower cut (y_min) is not defined !!! \n\n" <<std::endl;
        return 1;
    }
    
    // Check rapidity upper cut parameter:
    std::cout << checkParam("y_max") << std::endl;
    if ( ! checkParam("y_max") ) {
        std::cout << "\n\n FATAL ERROR: di-lepton rapidity upper cut (y_max) is not defined !!! \n\n" <<std::endl;
        return 1;
    }
    
    // Constant
    PI = 3.14159265;
    
    // Read default parameters
    GeVtofb_param = pow(10, -3) * GetParam("convFac");
    alphaEM_param = GetParam("alphaem");
    stheta2W_param = GetParam("sin2thW");
    MZ_param = GetParam("Mz");
    GammaZ_param = GetParam("Wz");

    // Read "reactions/AFB/yaml/params.yaml"
    energy_param = GetParam("energy");
    eta_cut_param = GetParam("eta_cut");
    pT_cut_param = GetParam("pT_cut");
    y_min_param = GetParam("y_min"); // not implemented yet
    y_max_param = GetParam("y_max"); // not implemented yet
    
    // Calculate fixed parameters
    e_param = sqrt(4*PI*alphaEM_param);
    gsm_param = (e_param/(sqrt(stheta2W_param)*sqrt(1-stheta2W_param)))*sqrt(1+pow(stheta2W_param,2));
    smangle_param = atan(-stheta2W_param);
    
    // Foton couplings
    foton_Vu = e_param*(2.0/3.0);
    foton_Au = 0;
    foton_Vd = e_param*(-1.0/3.0);
    foton_Ad = 0;
    foton_Vl = e_param*(-1.0);
    foton_Al = 0;
    foton_Vnu = 0;
    foton_Anu = 0;

    // Z-boson couplings
    Z_Vu = (1.0/2.0)*gsm_param*(1.0/6.0)*(3*cos(smangle_param)+8*sin(smangle_param));
    Z_Au = (1.0/2.0)*gsm_param*(cos(smangle_param)/2.0);
    Z_Vd = (1.0/2.0)*gsm_param*(1.0/6.0)*(-3*cos(smangle_param)-4*sin(smangle_param));
    Z_Ad = (1.0/2.0)*gsm_param*(-cos(smangle_param)/2.0);
    Z_Vl = (1.0/2.0)*gsm_param*((-cos(smangle_param)/2.0)+(-2*sin(smangle_param)));
    Z_Al = (1.0/2.0)*gsm_param*(-cos(smangle_param)/2.0);
    Z_Vnu = (1.0/2.0)*gsm_param*(cos(smangle_param)/2.0);
    Z_Anu = (1.0/2.0)*gsm_param*(cos(smangle_param)/2.0);
    
    // Even combination of couplings
    even_foton_up = (pow(foton_Vu,2)+pow(foton_Au,2))*(pow(foton_Vl,2)+pow(foton_Al,2));
    even_foton_down = (pow(foton_Vd,2)+pow(foton_Ad,2))*(pow(foton_Vl,2)+pow(foton_Al,2));
    even_interf_up = ((foton_Vu*Z_Vu)+(foton_Au*Z_Au))*((foton_Vl*Z_Vl)+(foton_Al*Z_Al));
    even_interf_down = ((foton_Vd*Z_Vd)+(foton_Ad*Z_Ad))*((foton_Vl*Z_Vl)+(foton_Al*Z_Al));
    even_Z_up = (pow(Z_Vu,2)+pow(Z_Au,2))*(pow(Z_Vl,2)+pow(Z_Al,2));
    even_Z_down = (pow(Z_Vd,2)+pow(Z_Ad,2))*(pow(Z_Vl,2)+pow(Z_Al,2));

    // Odd combination of couplings
    odd_foton_up = 4*foton_Vu*foton_Au*foton_Vl*foton_Al;
    odd_foton_down = 4*foton_Vd*foton_Ad*foton_Vl*foton_Al;
    odd_interf_up = (foton_Vu*Z_Au+foton_Au*Z_Vu)*(foton_Vl*Z_Al+foton_Al*Z_Vl);
    odd_interf_down = (foton_Vd*Z_Ad+foton_Ad*Z_Vd)*(foton_Vl*Z_Al+foton_Al*Z_Vl);
    odd_Z_up = 4*Z_Vu*Z_Au*Z_Vl*Z_Al;
    odd_Z_down = 4*Z_Vd*Z_Ad*Z_Vl*Z_Al;

    return 0;
}

// Main function to compute results at an iteration
int ReactionAFB::compute(int dataSetID, valarray<double> &val, map<string, valarray<double> > &err)
{
    auto *Minv_min  = GetBinValues(dataSetID,"Minv_min"), *Minv_max  = GetBinValues(dataSetID,"Minv_max");  
    if (Minv_min == nullptr || Minv_max == nullptr) {
    std::cout << "\n\nFATAL ERROR: AFB code requires Invariant mass bins to be present !!!" << std::endl;
    std::cout << "CHECK THE DATAFILE !!!" << std::endl;
    return 1;
    }

    auto min = *Minv_min, max = *Minv_max;

    int Npnt_min = min.size();
    int Npnt_max = max.size();

    if (Npnt_min != Npnt_max) {
        std::cout << "\n\nFATAL ERROR: uneven number of Invariant mass min and max !!!" << std::endl;
        std::cout << "CHECK THE DATAFILE !!!" << std::endl;
        return 1;
    }    
    
    // Fill the array "val[i]" with the result of the AFB function
    for (int i = 0; i < Npnt_min; i++) {
    double AFB_result = AFB (min[i], max[i]);
    val[i] = AFB_result;
    }

    return 0;
}
