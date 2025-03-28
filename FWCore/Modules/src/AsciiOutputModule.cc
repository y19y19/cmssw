/*----------------------------------------------------------------------
----------------------------------------------------------------------*/

#include <algorithm>
#include <iterator>
#include <ostream>
#include <iostream>
#include <string>
#include "FWCore/Framework/interface/global/OutputModule.h"
#include "FWCore/Framework/interface/EventForOutput.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "DataFormats/Provenance/interface/Provenance.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ParameterSet/interface/ConfigurationDescriptions.h"
#include "FWCore/ParameterSet/interface/ParameterSetDescription.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "FWCore/ParameterSet/interface/Registry.h"

namespace edm {

  class ModuleCallingContext;

  class AsciiOutputModule : public global::OutputModule<> {
  public:
    // We do not take ownership of passed stream.
    explicit AsciiOutputModule(ParameterSet const& pset);
    ~AsciiOutputModule() override;
    static void fillDescriptions(ConfigurationDescriptions& descriptions);

  private:
    void write(EventForOutput const& e) override;
    void writeLuminosityBlock(LuminosityBlockForOutput const&) override {}
    void writeRun(RunForOutput const&) override {}
    int prescale_;
    int verbosity_;
    int counter_;
    bool allProvenance_;
  };

  AsciiOutputModule::AsciiOutputModule(ParameterSet const& pset)
      : global::OutputModuleBase(pset),
        global::OutputModule<>(pset),
        prescale_(pset.getUntrackedParameter<unsigned int>("prescale")),
        verbosity_(pset.getUntrackedParameter<unsigned int>("verbosity")),
        counter_(0),
        allProvenance_(pset.getUntrackedParameter<bool>("allProvenance")) {
    if (prescale_ == 0)
      prescale_ = 1;
  }

  AsciiOutputModule::~AsciiOutputModule() {
    LogAbsolute("AsciiOut") << ">>> processed " << counter_ << " events" << std::endl;
  }

  void AsciiOutputModule::write(EventForOutput const& e) {
    if ((++counter_ % prescale_) != 0 || verbosity_ <= 0)
      return;

    // RunForOutput const& run = evt.getRun(); // this is still unused
    LogAbsolute("AsciiOut") << ">>> processing event # " << e.id() << " time " << e.time().value() << std::endl;

    if (verbosity_ <= 1)
      return;

    // Write out non-EDProduct contents...

    // ... list of process-names
    for (auto const& process : e.processHistory()) {
      LogAbsolute("AsciiOut") << process.processName() << " ";
    }

    // ... collision id
    LogAbsolute("AsciiOut") << '\n' << e.id() << '\n';

    // Loop over products, and write some output for each...
    for (auto const& prod : e.productRegistry().productList()) {
      ProductDescription const& desc = prod.second;
      if (selected(desc)) {
        if (desc.isAlias()) {
          LogAbsolute("AsciiOut") << "ModuleLabel " << desc.moduleLabel() << " is an alias for";
        }

        auto const& prov = e.getProvenance(desc.originalBranchID());
        LogAbsolute("AsciiOut") << prov;

        if (verbosity_ > 2) {
          ProductDescription const& desc2 = prov.productDescription();
          std::string const& process = desc2.processName();
          std::string const& label = desc2.moduleLabel();
          ProcessHistory const& processHistory = e.processHistory();

          for (ProcessConfiguration const& pc : processHistory) {
            if (pc.processName() == process) {
              ParameterSetID const& psetID = pc.parameterSetID();
              pset::Registry const* psetRegistry = pset::Registry::instance();
              ParameterSet const* processPset = psetRegistry->getMapped(psetID);
              if (processPset) {
                if (desc.isAlias()) {
                  LogAbsolute("AsciiOut") << "Alias PSet\n" << processPset->getParameterSet(desc.moduleLabel());
                }
                LogAbsolute("AsciiOut") << processPset->getParameterSet(label) << "\n";
              }
            }
          }
        }
      } else if (allProvenance_) {
        auto const& prov = e.getStableProvenance(desc.originalBranchID());
        LogAbsolute("AsciiOut") << prov;
        if (verbosity_ > 2) {
          ProductDescription const& desc2 = prov.productDescription();
          std::string const& process = desc2.processName();
          std::string const& label = desc2.moduleLabel();
          ProcessHistory const& processHistory = e.processHistory();

          for (ProcessConfiguration const& pc : processHistory) {
            if (pc.processName() == process) {
              ParameterSetID const& psetID = pc.parameterSetID();
              pset::Registry const* psetRegistry = pset::Registry::instance();
              ParameterSet const* processPset = psetRegistry->getMapped(psetID);
              if (processPset) {
                if (desc.isAlias()) {
                  LogAbsolute("AsciiOut") << "Alias PSet\n" << processPset->getParameterSet(desc.moduleLabel());
                }
                LogAbsolute("AsciiOut") << processPset->getParameterSet(label) << "\n";
              }
            }
          }
        }
      }
    }
  }

  void AsciiOutputModule::fillDescriptions(ConfigurationDescriptions& descriptions) {
    ParameterSetDescription desc;
    desc.setComment("Outputs event information into text file.");
    desc.addUntracked("prescale", 1U)->setComment("prescale factor");
    desc.addUntracked("verbosity", 1U)
        ->setComment(
            "0: no output\n"
            "1: event ID and timestamp only\n"
            "2: provenance for each kept product\n"
            ">2: PSet and provenance for each kept product");
    desc.addUntracked("allProvenance", false)
        ->setComment("when printing provenance info, also print stable provenance of non-kept data products.");
    OutputModule::fillDescription(desc);
    descriptions.add("asciiOutput", desc);
  }
}  // namespace edm

using edm::AsciiOutputModule;
DEFINE_FWK_MODULE(AsciiOutputModule);
