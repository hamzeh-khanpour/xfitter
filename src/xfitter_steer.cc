#include "xfitter_steer.h"
#include "xfitter_pars.h"
#include "xfitter_cpp_base.h"
#include"xfitter_cpp.h"

#include "BaseEvolution.h"
#include "BasePdfDecomposition.h"
#include "BaseMinimizer.h"
#include <dlfcn.h>
#include <iostream>
#include<fstream>
#include <yaml-cpp/yaml.h>
#include <Profiler.h>
using std::string;
using std::cerr;

extern std::map<string,string> gReactionLibs;

void*createDynamicObject(const string&classname,const string&instanceName){
  //instantiate an object from a shared library
  //Used to create evolution, decomposition, parameterisation, could be used to create minimizer
  string libpath;
  try{
    libpath=PREFIX+string("/lib/")+gReactionLibs.at(classname);
  }catch(const std::out_of_range&ex){
    std::ostringstream s;
    if(gReactionLibs.count(classname)==0){
      cerr<<"[ERROR] Unknown dynamically loaded class \""<<classname<<"\""
          "\nMake sure that "<<PREFIX<<"/lib/Reactions.txt has an entry for this class"
          "\n[/ERRROR]"<<endl;
      s<<"F: Unknown dynamically loaded class \""<<classname<<"\", see stderr";
      hf_errlog(18091901,s.str().c_str());
    }
    cerr<<"[ERROR] Unknown out_of_range in function "<<__func__<<":\n"<<ex.what()<<"\n[/ERROR]\n";
    s<<"F: Unknown out_of_range exception in "<<__func__<<", see stderr";
    hf_errlog(18091902,s.str().c_str());
  }
  void*shared_library=dlopen(libpath.c_str(),RTLD_NOW);
  //By the way, do we ever call dlclose? I don't think so... Maybe we should call it eventually. --Ivan Novikov
  if(shared_library==NULL){
    std::cerr<<"[ERROR] dlopen() error while trying to open shared library for class \""<<classname<<"\":\n"<<dlerror()<<"\n[/ERROR]"<<std::endl;
    hf_errlog(18091900,"F:dlopen() error, see stderr");
  }
  //reset errors
  dlerror();
  void*create=dlsym(shared_library,"create");
  if(create==NULL){
    std::cerr<<"[ERROR] dlsym() failed to find \"create\" function for class \""<<classname<<"\":\n"<<dlerror()<<"\n[/ERROR]"<<std::endl;
    hf_errlog(18091902,"F:dlsym() error, see stderr");
  }
  return((void*(*)(const char*))create)(instanceName.c_str());
}

namespace xfitter {
  BaseEvolution*get_evolution(string name){
    if(name=="")name=XFITTER_PARS::getDefaultEvolutionName();
    // Check if already present
    if(XFITTER_PARS::gEvolutions.count(name)==1){
      return XFITTER_PARS::gEvolutions.at(name);
    }
    // Else create a new instance of evolution
    YAML::Node instanceNode=XFITTER_PARS::getEvolutionNode(name);
    YAML::Node classnameNode=instanceNode["class"];
    if(!classnameNode.IsScalar()){
      std::ostringstream s;
      s<<"F:Failed to get evolution \""<<name<<"\": evolution must have a node \"class\" with the class name as a string";
      hf_errlog(18082950,s.str().c_str());
    }
    string classname=classnameNode.as<string>();
    BaseEvolution*evolution=(BaseEvolution*)createDynamicObject(classname,name);
    //Note that unlike in the pervious version of this function, we do not set decompositions for evolutions
    //Evolution objects are expected to get their decomposition themselves based on YAML parameters, during atStart
    evolution->atStart();
    // Store the newly created evolution on the global map
    XFITTER_PARS::gEvolutions[name] = evolution;
    return evolution;
  }
  BasePdfDecomposition*get_pdfDecomposition(string name){
    try{
      if(name=="")name=XFITTER_PARS::getDefaultDecompositionName();
      auto it=XFITTER_PARS::gPdfDecompositions.find(name);
      if(it!=XFITTER_PARS::gPdfDecompositions.end())return it->second;
      string classname=XFITTER_PARS::getDecompositionNode(name)["class"].as<string>();
      BasePdfDecomposition*ret=(BasePdfDecomposition*)createDynamicObject(classname,name);
      ret->atStart();
      XFITTER_PARS::gPdfDecompositions[name]=ret;
      return ret;
    }catch(YAML::InvalidNode&ex){
      const int errcode=18092401;
      const char*errmsg="F: YAML::InvalidNode exception while creating decomposition, details written to stderr";
      using namespace std;
      cerr<<"[ERROR]"<<__func__<<"(\""<<name<<"\")"<<endl;
      YAML::Node node=XFITTER_PARS::getDecompositionNode(name);
      if(!node.IsMap()){
        cerr<<"Invalid node Decompositions/"<<name<<"\nnode is not a map\n[/ERROR]"<<endl;
        hf_errlog(errcode,errmsg);
      }
      YAML::Node node_class=node["class"];
      if(!node_class.IsScalar()){
        if(node_class.IsNull())cerr<<"Missing node Decompositions/"<<name<<"/class";
        else cerr<<"Invalid node Decompositions/"<<name<<"/class\nnode is not a scalar";
        cerr<<"\n[/ERROR]"<<endl;
        hf_errlog(errcode,errmsg);
      }
      cerr<<"Unexpected YAML exception\nNode:\n"<<node<<"\n[/ERROR]"<<endl;
      throw ex;
      //hf_errlog(errcode,errmsg);
    }
  }
  BasePdfParam*getParameterisation(const string&name){
    try{
      auto it=XFITTER_PARS::gParameterisations.find(name);
      if(it!=XFITTER_PARS::gParameterisations.end())return it->second;
      //Else create a new instance
      string classname=XFITTER_PARS::getParameterisationNode(name)["class"].as<string>();
      BasePdfParam*ret=(BasePdfParam*)createDynamicObject(classname,name);
      ret->atStart();
      XFITTER_PARS::gParameterisations[name]=ret;
      return ret;
    }catch(YAML::InvalidNode&ex){
      const int errcode=18092400;
      const char*errmsg="F: YAML::InvalidNode exception while creating parameterisation, details written to stderr";
      using namespace std;
      cerr<<"[ERROR]"<<__func__<<'('<<name<<')'<<endl;
      YAML::Node node=XFITTER_PARS::getParameterisationNode(name);
      if(!node.IsMap()){
        cerr<<"Invalid node Parameterisations/"<<name<<"\nnode is not a map\n[/ERROR]"<<endl;
        hf_errlog(errcode,errmsg);
      }
      YAML::Node node_class=node["class"];
      if(!node_class.IsScalar()){
        if(node_class.IsNull())cerr<<"Missing node Parameterisations/"<<name<<"/class";
        else cerr<<"Invalid node Parameterisations/"<<name<<"/class\nnode is not a scalar";
        cerr<<"\n[/ERROR]"<<endl;
        hf_errlog(errcode,errmsg);
      }
      cerr<<"Unexpected YAML exception\nNode:\n"<<node<<"\n[/ERROR]"<<endl;
      hf_errlog(errcode,errmsg);
    }
  }


  BaseMinimizer* get_minimizer() {
    std::string name = XFITTER_PARS::getParamS("Minimizer");
    
    // Check if already present
    if (XFITTER_PARS::gMinimizer != nullptr ) {
      return  XFITTER_PARS::gMinimizer;  //already loaded
    }
    
    // Load corresponding shared library:
    string libname = gReactionLibs[name];
    if ( libname == "") {
      hf_errlog(18081701,"F: Shared library for minimizer "+name+" not found");
    }

    // load the library:
    void *lib_handler = dlopen((PREFIX+string("/lib/")+libname).c_str(), RTLD_NOW);
    if ( lib_handler == nullptr )  { 
      std::cout  << dlerror() << std::endl;      
      hf_errlog(18081702,"F: Minimizer shared library ./lib/"  + libname  +  " not present for minimizer" + name + ". Check Reactions.txt file");
    }

         // reset errors
    dlerror();

    create_minimizer *dispatch_minimizer = (create_minimizer*) dlsym(lib_handler, "create");
    BaseMinimizer *minimizer = dispatch_minimizer();
    minimizer->atStart();

    // store on the map
    XFITTER_PARS::gMinimizer = minimizer;

    return XFITTER_PARS::gMinimizer;
  }

}


/// Temporary interface for fortran
extern "C" {
  void init_evolution_(); 
  void init_minimizer_();
  void run_minimizer_();
  void report_convergence_status_();
  void run_error_analysis_();
}

namespace xfitter{
  BaseEvolution*defaultEvolution=nullptr;//declared in xfitter_steer.h
}
//Make sure default evolution exists
void init_evolution_() {
  xfitter::defaultEvolution=xfitter::get_evolution();
}

void init_minimizer_() {
  /// atStart is called inside
  auto mini = xfitter::get_minimizer();
}

void run_minimizer_() {
  auto mini = xfitter::get_minimizer();
  /// get profiler too
  auto *prof = new xfitter::Profiler();

  prof->doProfiling();

  mini->doMinimization();
}

void report_convergence_status_(){
  //Get a status code from current minimizer and log a message, write status to file Status.out
  using namespace xfitter;
  auto status=get_minimizer()->convergenceStatus();
  //Write status to Status.out
  {
  std::ofstream f;
  f.open(stringFromFortran(coutdirname_.outdirname,sizeof(coutdirname_.outdirname))+"/Status.out");
  if(!f.is_open()){
    hf_errlog(16042807,"W: Failed to open Status.out for writing");
    return;
  }
  if(status==ConvergenceStatus::SUCCESS)f<<"OK";
  else f<<"Failed";
  f.close();
  }
  //Log status message
  switch(status){
    case ConvergenceStatus::NORUN:
      hf_errlog(16042801,"I: No minimization has run");
      break;
    case ConvergenceStatus::INACCURATE:
      hf_errlog(16042803,"S: Error matrix not accurate");
      break;
    case ConvergenceStatus::FORCED_POSITIVE:
      hf_errlog(16042804,"S: Error matrix forced positive");
      break;
    case ConvergenceStatus::SUCCESS:
      hf_errlog(16042802,"I: Fit converged");
      break;
    case ConvergenceStatus::NO_CONVERGENCE:
      hf_errlog(16042805,"S: No convergence");
      break;
    case ConvergenceStatus::ERROR:
      hf_errlog(16042806,"F: Minimizer error");
      break;
  }
}

void run_error_analysis_() {
  auto mini = xfitter::get_minimizer();
  mini->errorAnalysis();    
}

namespace xfitter{
void updateAtConfigurationChange(){
  //Call atConfigurationChange for each evolution and for each decomposition
  for(map<string,BaseEvolution*>::const_iterator it=XFITTER_PARS::gEvolutions.begin();it!=XFITTER_PARS::gEvolutions.end();++it){
    it->second->atConfigurationChange();
  }
  for(map<string,BasePdfDecomposition*>::const_iterator it=XFITTER_PARS::gPdfDecompositions.begin();it!=XFITTER_PARS::gPdfDecompositions.end();++it){
    it->second->atConfigurationChange();
  }
}
}