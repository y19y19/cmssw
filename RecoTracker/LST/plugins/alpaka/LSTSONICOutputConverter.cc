#include <alpaka/alpaka.hpp>
//#include <algorithm> //YY: maybe we need it for copy_n
#include "RecoTracker/LSTCore/interface/alpaka/LST.h"

#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "FWCore/ParameterSet/interface/ConfigurationDescriptions.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ParameterSet/interface/ParameterSetDescription.h"
#include "FWCore/Utilities/interface/InputTag.h"

#include "HeterogeneousCore/AlpakaCore/interface/alpaka/EDGetToken.h"
#include "HeterogeneousCore/AlpakaCore/interface/alpaka/EDPutToken.h"
#include "HeterogeneousCore/AlpakaCore/interface/alpaka/Event.h"
#include "HeterogeneousCore/AlpakaCore/interface/alpaka/EventSetup.h"
#include "HeterogeneousCore/AlpakaCore/interface/alpaka/global/EDProducer.h"
#include "HeterogeneousCore/AlpakaInterface/interface/config.h"

#include "RecoTracker/LSTCore/interface/alpaka/TrackCandidatesDeviceCollection.h"
#include "RecoTracker/LSTCore/interface/TrackCandidatesHostCollection.h"
#include "RecoTracker/LSTCore/interface/TrackCandidatesSoA.h"

#include "RecoTracker/Record/interface/TrackerRecoGeometryRecord.h"

namespace ALPAKA_ACCELERATOR_NAMESPACE {

  class LSTSONICOutputConverter : public global::EDProducer<> {
  public:
    LSTSONICOutputConverter(edm::ParameterSet const& config)
        : EDProducer(config),
          nTrackCandidatesToken_{consumes(config.getParameter<edm::InputTag>("nTrackCandidates"))},
          pixelSeedIndexToken_{consumes(config.getParameter<edm::InputTag>("pixelSeedIndex"))},
          trackCandidateTypeToken_{consumes(config.getParameter<edm::InputTag>("trackCandidateType"))},
          hitIndicesToken_{consumes(config.getParameter<edm::InputTag>("hitIndices"))},

          lstOutputToken_{produces()} {}

    void produce(edm::StreamID sid, device::Event& iEvent, const device::EventSetup& iSetup) const override {

      // YY: break point
      std::cout << "YY: Start LSTSONICOutputConverter::produce" << std::endl;

      // Convert output to lstOutputHC (SOA) !! this needs to be done in the alpaka folder 
      auto const& nTrackCandidates = iEvent.get(nTrackCandidatesToken_); // this is an integer
      auto const& pixelSeedIndex = iEvent.get(pixelSeedIndexToken_); // this is a std::vector<int>?
      auto const& trackCandidateType = iEvent.get(trackCandidateTypeToken_);
      auto const& hitIndices_2D = iEvent.get(hitIndicesToken_);

      lst::TrackCandidatesBaseHostCollection lstOutputHC(iEvent.queue(), nTrackCandidates); // May not work... if it needs an alpaka queue... 
      //auto lstTrackCandidates = lstOutputHC.view<lst::TrackCandidatesBaseSoA>();
      auto lstTrackCandidates = lstOutputHC.view();
      //std::memcpy(lstTrackCandidates.nTrackCandidates(), nTrackCandidates, 1.0 * sizeof(unsigned int));
      std::memcpy(lstTrackCandidates.pixelSeedIndex().data(), pixelSeedIndex.data(), pixelSeedIndex.size() * sizeof(unsigned int));
      std::memcpy(lstTrackCandidates.trackCandidateType().data(), trackCandidateType.data(), trackCandidateType.size() * sizeof(int8_t)); // YY: using this instead of copy_n because it cannot figure out the size of trackCandidateType by itself
      std::memcpy(lstTrackCandidates.hitIndices().data(), hitIndices_2D.data(), hitIndices_2D.size() * sizeof(lst::Params_TC::ArrayUxHits)); // If does not work, use kHits * sizeof(unsigned int)
      lstTrackCandidates.nTrackCandidates() = nTrackCandidates;
      //std::copy_n(pixelSeedIndex.data(), pixelSeedIndex.size(), lstTrackCandidates.pixelSeedIndex().data()); // YY: std::copy_n(src, n, dst);
      //std::copy_n(trackCandidateType.data(), trackCandidateType.size(), lstTrackCandidates.trackCandidateType().data());
      //std::copy_n(hitIndices_2D.data(), hitIndices_2D.size(), lstTrackCandidates.hitIndices().data());  
      // Output
      //iEvent.emplace(lstOutputToken_, std::move(*lstTrackCandidates.release()));
      iEvent.emplace(lstOutputToken_, std::move(lstOutputHC));
    }

    static void fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
      edm::ParameterSetDescription desc;

      desc.add<edm::InputTag>("nTrackCandidates", edm::InputTag("lstSONICProducer"));
      desc.add<edm::InputTag>("pixelSeedIndex", edm::InputTag("lstSONICProducer"));
      desc.add<edm::InputTag>("trackCandidateType", edm::InputTag("lstSONICProducer"));
      desc.add<edm::InputTag>("hitIndices", edm::InputTag("lstSONICProducer"));

      descriptions.addWithDefaultLabel(desc);
    }

  private:
    const edm::EDGetTokenT<unsigned int> nTrackCandidatesToken_;
    const edm::EDGetTokenT<std::vector<unsigned int>> pixelSeedIndexToken_;
    const edm::EDGetTokenT<std::vector<int8_t>> trackCandidateTypeToken_;
    const edm::EDGetTokenT<std::vector<lst::Params_TC::ArrayUxHits>> hitIndicesToken_;

    //const device::EDPutToken<lst::TrackCandidatesBaseDeviceCollection> lstOutputToken_;
    const edm::EDPutTokenT<lst::TrackCandidatesBaseHostCollection> lstOutputToken_;
  };

}  // namespace ALPAKA_ACCELERATOR_NAMESPACE

#include "HeterogeneousCore/AlpakaCore/interface/alpaka/MakerMacros.h"
DEFINE_FWK_ALPAKA_MODULE(LSTSONICOutputConverter);
