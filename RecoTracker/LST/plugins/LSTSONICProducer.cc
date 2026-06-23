//#include <alpaka/alpaka.hpp>
#include <fstream>
#include <sstream>
#include <string>

#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "FWCore/ParameterSet/interface/ConfigurationDescriptions.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ParameterSet/interface/ParameterSetDescription.h"
#include "FWCore/Utilities/interface/InputTag.h"
#include "FWCore/Utilities/interface/transform.h"
#include "FWCore/Framework/interface/MakerMacros.h"


//#include "HeterogeneousCore/AlpakaCore/interface/alpaka/EDGetToken.h"
//#include "HeterogeneousCore/AlpakaCore/interface/alpaka/EDPutToken.h"
//#include "HeterogeneousCore/AlpakaCore/interface/alpaka/ESGetToken.h"
//#include "HeterogeneousCore/AlpakaCore/interface/alpaka/Event.h"
//#include "HeterogeneousCore/AlpakaCore/interface/alpaka/EventSetup.h"
//#include "HeterogeneousCore/AlpakaCore/interface/alpaka/MakerMacros.h"
//#include "HeterogeneousCore/AlpakaCore/interface/alpaka/global/EDProducer.h"
//#include "HeterogeneousCore/AlpakaInterface/interface/config.h"

#include "MagneticField/Engine/interface/MagneticField.h"
#include "MagneticField/Records/interface/IdealMagneticFieldRecord.h"

#include "DataFormats/TrackerRecHit2D/interface/SiStripMatchedRecHit2DCollection.h"
#include "DataFormats/TrackerRecHit2D/interface/Phase2TrackerRecHit1D.h"
#include "DataFormats/TrajectorySeed/interface/TrajectorySeedCollection.h"
#include "DataFormats/TrackReco/interface/trackFromSeedFitFailed.h"

#include "TrackingTools/Records/interface/TransientRecHitRecord.h"
#include "TrackingTools/TrajectoryState/interface/TrajectoryStateTransform.h"
#include "TrackingTools/TransientTrackingRecHit/interface/TransientTrackingRecHitBuilder.h"

//#include "RecoTracker/LSTCore/interface/LSTInputHostCollection.h"
//#include "RecoTracker/LSTCore/interface/LSTPrepareInput.h"
//#include "RecoTracker/LSTCore/interface/alpaka/LST.h"
//#include "RecoTracker/LSTCore/interface/alpaka/TrackCandidatesDeviceCollection.h"
#include "RecoTracker/LSTCore/interface/Common.h"
#include "RecoTracker/LSTCore/interface/LSTInputHostCollection.h"
#include "RecoTracker/LSTCore/interface/TrackCandidatesHostCollection.h"

#include "RecoTracker/Record/interface/TrackerRecoGeometryRecord.h"

#include "HeterogeneousCore/SonicTriton/interface/TritonEDProducer.h"


class LSTSONICProducer : public TritonEDProducer<> { 
public:
  
  LSTSONICProducer(edm::ParameterSet const& config) 
    : TritonEDProducer<>(config),
    //lstInputToken_{consumes(config.getParameter<edm::InputTag>("lstInput"))}, // YY: do not take lstInput because it is a HC

    phase2OTRecHitToken_(consumes(config.getParameter<edm::InputTag>("phase2OTRecHits"))),
    mfToken_(esConsumes()), // YY: it is empty?
    beamSpotToken_(consumes(config.getParameter<edm::InputTag>("beamSpot"))),
    seedTokens_(
      edm::vector_transform(config.getParameter<std::vector<edm::InputTag>>("seedTracks"),
                            [&](const edm::InputTag& tag) { return consumes<edm::View<reco::Track>>(tag); })),
    //lstESToken_{esConsumes(edm::ESInputTag("", config.getParameter<std::string>("ptCutLabel")))}, // YY: might not be needed
    verbose_(config.getParameter<bool>("verbose")),
    ptCut_(config.getParameter<double>("ptCut")),
    nopLSDupClean_(config.getParameter<bool>("nopLSDupClean")),
    tcpLSTriplets_(config.getParameter<bool>("tcpLSTriplets")),

    nTrackCandidatesPutToken_{produces()}, 
    pixelSeedIndexPutToken_{produces()}, 
    trackCandidateTypePutToken_{produces()}, 
    hitIndicesPutToken_{produces()}
    {}
    
  //~LSTSONICProducer() override = default;

  void acquire(edm::Event const& iEvent, edm::EventSetup const& iSetup, Input& iInput);

  void produce(edm::Event& iEvent, edm::EventSetup const& iSetup, Output const& iOutput);

  static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);

private:

  const edm::EDGetTokenT<Phase2TrackerRecHit1DCollectionNew> phase2OTRecHitToken_;
  const edm::ESGetToken<MagneticField, IdealMagneticFieldRecord> mfToken_;
  const edm::EDGetTokenT<reco::BeamSpot> beamSpotToken_;
  const std::vector<edm::EDGetTokenT<edm::View<reco::Track>>> seedTokens_;
  //const device::ESGetToken<lst::LSTESData<Device>, TrackerRecoGeometryRecord> lstESToken_;
  const bool verbose_;
  const double ptCut_;
  const bool nopLSDupClean_;
  const bool tcpLSTriplets_;
  const edm::EDPutTokenT<unsigned int> nTrackCandidatesPutToken_;
  const edm::EDPutTokenT<std::vector<unsigned int>> pixelSeedIndexPutToken_;
  const edm::EDPutTokenT<std::vector<int8_t>> trackCandidateTypePutToken_;
  const edm::EDPutTokenT<std::vector<lst::Params_TC::ArrayUxHits>> hitIndicesPutToken_;
};

void LSTSONICProducer::acquire(edm::Event const& iEvent, edm::EventSetup const& iSetup, Input& iInput) {

  // YY: break point
  std::cout << "YY: Start LSTSONICProducer::acquire" << std::endl;

  auto& input_see_px = iInput.at("see_px");
  auto& input_see_py = iInput.at("see_py");
  auto& input_see_pz = iInput.at("see_pz");
  auto& input_see_dxy = iInput.at("see_dxy");
  auto& input_see_dz = iInput.at("see_dz");
  auto& input_see_ptErr = iInput.at("see_ptErr");
  auto& input_see_etaErr = iInput.at("see_etaErr");
  auto& input_see_stateTrajGlbX = iInput.at("see_stateTrajGlbX");
  auto& input_see_stateTrajGlbY = iInput.at("see_stateTrajGlbY");
  auto& input_see_stateTrajGlbZ = iInput.at("see_stateTrajGlbZ");
  auto& input_see_stateTrajGlbPx = iInput.at("see_stateTrajGlbPx");
  auto& input_see_stateTrajGlbPy = iInput.at("see_stateTrajGlbPy");
  auto& input_see_stateTrajGlbPz = iInput.at("see_stateTrajGlbPz");
  auto& input_see_q = iInput.at("see_q");
  auto& input_see_hit_size = iInput.at("see_hit_size");
  auto& input_see_hitIdx = iInput.at("see_hitIdx");
  auto& input_see_hitType = iInput.at("see_hitType");
  auto& input_see_algo = iInput.at("see_algo");
  auto& input_ph2_detId = iInput.at("ph2_detId");
  auto& input_ph2_clustSize = iInput.at("ph2_clustSize");
  auto& input_ph2_x = iInput.at("ph2_x");
  auto& input_ph2_y = iInput.at("ph2_y");
  auto& input_ph2_z = iInput.at("ph2_z");
  auto& input_ptCut = iInput.at("ptCut");
  auto& input_clustSizeCut = iInput.at("clustSizeCut");

  auto inputdata_see_px = input_see_px.allocate<float>(); // std::shared_ptr<std::vector<std::vector<float>>>
  auto inputdata_see_py = input_see_py.allocate<float>();
  auto inputdata_see_pz = input_see_pz.allocate<float>();
  auto inputdata_see_dxy = input_see_dxy.allocate<float>();
  auto inputdata_see_dz = input_see_dz.allocate<float>();
  auto inputdata_see_ptErr = input_see_ptErr.allocate<float>();
  auto inputdata_see_etaErr = input_see_etaErr.allocate<float>();
  auto inputdata_see_stateTrajGlbX = input_see_stateTrajGlbX.allocate<float>();
  auto inputdata_see_stateTrajGlbY = input_see_stateTrajGlbY.allocate<float>();
  auto inputdata_see_stateTrajGlbZ = input_see_stateTrajGlbZ.allocate<float>();
  auto inputdata_see_stateTrajGlbPx = input_see_stateTrajGlbPx.allocate<float>();
  auto inputdata_see_stateTrajGlbPy = input_see_stateTrajGlbPy.allocate<float>();
  auto inputdata_see_stateTrajGlbPz = input_see_stateTrajGlbPz.allocate<float>();
  auto inputdata_see_q = input_see_q.allocate<int>();
  auto inputdata_see_hit_size = input_see_hit_size.allocate<unsigned int>();
  auto inputdata_see_hitIdx = input_see_hitIdx.allocate<int>();
  auto inputdata_see_hitType = input_see_hitType.allocate<int>();
  auto inputdata_see_algo = input_see_algo.allocate<unsigned int>();
  auto inputdata_ph2_detId = input_ph2_detId.allocate<unsigned int>();
  auto inputdata_ph2_clustSize = input_ph2_clustSize.allocate<uint16_t>();
  auto inputdata_ph2_x = input_ph2_x.allocate<float>();
  auto inputdata_ph2_y = input_ph2_y.allocate<float>();
  auto inputdata_ph2_z = input_ph2_z.allocate<float>();
  auto inputdata_ptCut = input_ptCut.allocate<float>();
  auto inputdata_clustSizeCut = input_clustSizeCut.allocate<uint16_t>();

  auto& see_px = (*inputdata_see_px)[0]; // what is this [0] mean? batch index // std::vector<float>
  auto& see_py = (*inputdata_see_py)[0];
  auto& see_pz = (*inputdata_see_pz)[0];
  auto& see_dxy = (*inputdata_see_dxy)[0];
  auto& see_dz = (*inputdata_see_dz)[0];
  auto& see_ptErr = (*inputdata_see_ptErr)[0];
  auto& see_etaErr = (*inputdata_see_etaErr)[0];
  auto& see_stateTrajGlbX = (*inputdata_see_stateTrajGlbX)[0];
  auto& see_stateTrajGlbY = (*inputdata_see_stateTrajGlbY)[0];
  auto& see_stateTrajGlbZ = (*inputdata_see_stateTrajGlbZ)[0];
  auto& see_stateTrajGlbPx = (*inputdata_see_stateTrajGlbPx)[0];
  auto& see_stateTrajGlbPy = (*inputdata_see_stateTrajGlbPy)[0];
  auto& see_stateTrajGlbPz = (*inputdata_see_stateTrajGlbPz)[0];
  auto& see_q = (*inputdata_see_q)[0];
  auto& see_hit_size = (*inputdata_see_hit_size)[0];
  auto& see_hitIdx = (*inputdata_see_hitIdx)[0];
  auto& see_hitType = (*inputdata_see_hitType)[0];
  auto& see_algo = (*inputdata_see_algo)[0];
  auto& ph2_detId = (*inputdata_ph2_detId)[0];
  auto& ph2_clustSize = (*inputdata_ph2_clustSize)[0];
  auto& ph2_x = (*inputdata_ph2_x)[0];
  auto& ph2_y = (*inputdata_ph2_y)[0];
  auto& ph2_z = (*inputdata_ph2_z)[0];
  auto& ptCut = (*inputdata_ptCut)[0];
  auto& clustSizeCut = (*inputdata_clustSizeCut)[0];

  // Get the ptCut info
  ptCut.push_back(ptCut_);
  clustSizeCut.push_back(static_cast<uint16_t>(16));

  // Get ph2 info 
  auto const& phase2OTHits = iEvent.get(phase2OTRecHitToken_);

  // YY: copied from LSTInputProducer, disgard the hits
  for (auto const& it : phase2OTHits) {
    const DetId hitId = it.detId();
    for (auto const& hit : it) {
      ph2_detId.push_back(hitId.rawId()); // YY: Do I need to do the ph2_detId.reserve(phase2OTHits.dataSize())?
      ph2_clustSize.push_back(hit.cluster()->size());
      ph2_x.push_back(hit.globalPosition().x());
      ph2_y.push_back(hit.globalPosition().y());
      ph2_z.push_back(hit.globalPosition().z()); // YY: ignore the hits
    }
  }

  // Get the pixel seeds
  auto const& mf = iSetup.getData(mfToken_);
  auto const& bs = iEvent.get(beamSpotToken_);

  // YY: the whole for-loop copied from LSTInputProducer, but change the see_hit_size, and disgard see_seeds
  for (auto const& seedToken : seedTokens_) {
    auto const& seedTracks = iEvent.get(seedToken);

    if (seedTracks.empty())
      continue;

    // Get seed track refs
    edm::RefToBaseVector<reco::Track> seedTrackRefs;
    for (edm::View<reco::Track>::size_type i = 0; i < seedTracks.size(); ++i) {
      seedTrackRefs.push_back(seedTracks.refAt(i));
    }

    edm::ProductID id = seedTracks[0].seedRef().id();

    for (size_t iSeed = 0; iSeed < seedTrackRefs.size(); ++iSeed) {
      auto const& seedTrackRef = seedTrackRefs[iSeed];
      auto const& seedTrack = *seedTrackRef;
      auto const& seedRef = seedTrack.seedRef();
      auto const& seed = *seedRef;

      if (seedRef.id() != id)
        throw cms::Exception("LogicError")
            << "All tracks in 'TracksFromSeeds' collection should point to seeds in the same collection. Now the "
                "element 0 had ProductID "
            << id << " while the element " << seedTrackRef.key() << " had " << seedTrackRef.id() << ".";

      const bool seedFitOk = !trackFromSeedFitFailed(seedTrack);

      const TrackingRecHit* lastRecHit = &*(seed.recHits().end() - 1);
      TrajectoryStateOnSurface tsos =
          trajectoryStateTransform::transientState(seed.startingState(), lastRecHit->surface(), &mf);
      auto const& stateGlobal = tsos.globalParameters();

      std::vector<int> hitIdx;
      std::vector<int> hitType;
      for (auto const& hit : seed.recHits()) {
        auto det = hit.geographicalId().det();
        if (det == DetId::Tracker) {
          const BaseTrackerRecHit* bhit = dynamic_cast<const BaseTrackerRecHit*>(&hit);
          const auto& clusterRef = bhit->firstClusterRef();
          hitIdx.push_back(clusterRef.index());
          if (clusterRef.isPixel()) {
            hitType.push_back(static_cast<int>(lst::HitType::Pixel));
          } else if (clusterRef.isPhase2()) {
            hitType.push_back(static_cast<int>(lst::HitType::Phase2OT));
          } else {
            throw cms::Exception("LSTSONICProducer") << "Unknown tracker hit type found!";
          }
        } else {
          throw cms::Exception("LSTSONICProducer") << "Not tracker hit found!";
        }
      }

      // Fill output
      see_px.push_back(seedFitOk ? seedTrack.px() : 0);
      see_py.push_back(seedFitOk ? seedTrack.py() : 0);
      see_pz.push_back(seedFitOk ? seedTrack.pz() : 0);
      see_dxy.push_back(seedFitOk ? seedTrack.dxy(bs.position()) : 0);
      see_dz.push_back(seedFitOk ? seedTrack.dz(bs.position()) : 0);
      see_ptErr.push_back(seedFitOk ? seedTrack.ptError() : 0);
      see_etaErr.push_back(seedFitOk ? seedTrack.etaError() : 0);
      see_stateTrajGlbX.push_back(stateGlobal.position().x());
      see_stateTrajGlbY.push_back(stateGlobal.position().y());
      see_stateTrajGlbZ.push_back(stateGlobal.position().z());
      see_stateTrajGlbPx.push_back(stateGlobal.momentum().x());
      see_stateTrajGlbPy.push_back(stateGlobal.momentum().y());
      see_stateTrajGlbPz.push_back(stateGlobal.momentum().z());
      see_q.push_back(seedTrack.charge());
      see_hit_size.push_back(hitIdx.size());
      see_hitIdx.insert(see_hitIdx.end(), hitIdx.begin(), hitIdx.end());
      see_hitType.insert(see_hitType.end(), hitType.begin(), hitType.end());
      // TrackFromSeedProducer doesn't set AlgorithmName so algo() returns 0.
      // Seeds are already pre-filtered to initialStep/highPtTripletStep; push 4
      // so prepareInput's good_seed_type filter (algo==4 || algo==22) passes.
      see_algo.push_back(4);
    }
  }
  /*
  std::cout << "YY: Input to server: " << std::endl;
  std::cout << "see_px : size "   << see_px.size()
            << " first element: " << see_px.front()
            << " last element "   << see_px.back() << std::endl;
  std::cout << "see_py : size "   << see_py.size()
            << " first element: " << see_py.front() 
            << " last element "   << see_py.back() << std::endl;
  std::cout << "see_pz : size "   << see_pz.size()
            << " first element: " << see_pz.front() 
            << " last element "   << see_pz.back() << std::endl;
  std::cout << "see_dxy : size "  << see_dxy.size()
            << " first element: " << see_dxy.front() 
            << " last element "   << see_dxy.back() << std::endl;
  std::cout << "see_dz : size "   << see_dz.size()
            << " first element: " << see_dz.front() 
            << " last element "   << see_dz.back() << std::endl;
  std::cout << "see_ptErr : size "<< see_ptErr.size()
            << " first element: " << see_ptErr.front() 
            << " last element "   << see_ptErr.back() << std::endl;
  std::cout << "see_etaErr : size " << see_etaErr.size()
            << " first element: " << see_etaErr.front() 
            << " last element "   << see_etaErr.back() << std::endl;
  std::cout << "see_stateTrajGlbX : size "   << see_stateTrajGlbX.size()
            << " first element: " << see_stateTrajGlbX.front()
            << " last element "   << see_stateTrajGlbX.back() << std::endl; 
  std::cout << "see_stateTrajGlbY : size "   << see_stateTrajGlbY.size()
            << " first element: " << see_stateTrajGlbY.front()
            << " last element "   << see_stateTrajGlbY.back() << std::endl; 
  std::cout << "see_stateTrajGlbZ : size "   << see_stateTrajGlbZ.size()
            << " first element: " << see_stateTrajGlbZ.front()
            << " last element "   << see_stateTrajGlbZ.back() << std::endl; 
  std::cout << "see_stateTrajGlbPx : size "   << see_stateTrajGlbPx.size()
            << " first element: " << see_stateTrajGlbPx.front()
            << " last element "   << see_stateTrajGlbPx.back() << std::endl;
  std::cout << "see_stateTrajGlbPy : size "   << see_stateTrajGlbPy.size()
            << " first element: " << see_stateTrajGlbPy.front()
            << " last element "   << see_stateTrajGlbPy.back() << std::endl; 
  std::cout << "see_stateTrajGlbPz : size "   << see_stateTrajGlbPz.size()
            << " first element: " << see_stateTrajGlbPz.front()
            << " last element "   << see_stateTrajGlbPz.back() << std::endl;
  std::cout << "see_q : size "   << see_q.size()
            << " first element: " << see_q.front()
            << " last element "   << see_q.back() << std::endl;
  std::cout << "see_hitIdx : size "   << see_hitIdx.size()
            << " first element: " << see_hitIdx.front()
            << " last element:"   << see_hitIdx.back() << std::endl;
  std::cout << "ph2_detId : size "   << ph2_detId.size()
            << " first element: " << ph2_detId.front()
            << " last element "   << ph2_detId.back() << std::endl;
  std::cout << "ph2_clustSize : size "   << ph2_clustSize.size()
            << " first element: " << ph2_clustSize.front()
            << " last element "   << ph2_clustSize.back() << std::endl;
  std::cout << "ph2_x : size "   << ph2_x.size()
            << " first element: " << ph2_x.front()
            << " last element "   << ph2_x.back() << std::endl;
  std::cout << "ph2_y : size "   << ph2_y.size()
            << " first element: " << ph2_y.front()
            << " last element "   << ph2_y.back() << std::endl;
  std::cout << "ph2_z : size "   << ph2_z.size()
            << " first element: " << ph2_z.front()
            << " last element "   << ph2_z.back() << std::endl;
  std::cout << "ptCut : value "   << ptCut_ << std::endl;
  */
  // Write this event's inputs to LST_input.json for perf_analyzer.
  // Format: {"data": [{event0}, {event1}, ...]}
  // Each call opens the file, appends one entry, and closes it.
  /* 
  {
    // Build the JSON object for this event's inputs.
    std::ostringstream ev;
    auto writeArr = [&](const char* name, auto const& vec, bool last = false) {
      ev << "      \"" << name << "\": [";
      for (size_t i = 0; i < vec.size(); ++i) {
        if (i > 0) ev << ", ";
        ev << +vec[i];
      }
      ev << "]";
      if (!last) ev << ",";
      ev << "\n";
    };
    // Like writeArr but guarantees a decimal point for FP32 fields so
    // perf_analyzer can identify them as float data (it rejects bare integers).
    auto writeFloatArr = [&](const char* name, auto const& vec, bool last = false) {
      ev << "      \"" << name << "\": [";
      for (size_t i = 0; i < vec.size(); ++i) {
        if (i > 0) ev << ", ";
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%g", static_cast<float>(vec[i]));
        std::string s(buf);
        if (s.find('.') == std::string::npos && s.find('e') == std::string::npos)
          s += ".0";
        ev << s;
      }
      ev << "]";
      if (!last) ev << ",";
      ev << "\n";
    };
    ev << "    {\n";
    writeFloatArr("see_px", see_px);
    writeFloatArr("see_py", see_py);
    writeFloatArr("see_pz", see_pz);
    writeFloatArr("see_dxy", see_dxy);
    writeFloatArr("see_dz", see_dz);
    writeFloatArr("see_ptErr", see_ptErr);
    writeFloatArr("see_etaErr", see_etaErr);
    writeFloatArr("see_stateTrajGlbX", see_stateTrajGlbX);
    writeFloatArr("see_stateTrajGlbY", see_stateTrajGlbY);
    writeFloatArr("see_stateTrajGlbZ", see_stateTrajGlbZ);
    writeFloatArr("see_stateTrajGlbPx", see_stateTrajGlbPx);
    writeFloatArr("see_stateTrajGlbPy", see_stateTrajGlbPy);
    writeFloatArr("see_stateTrajGlbPz", see_stateTrajGlbPz);
    writeArr("see_q", see_q);
    writeArr("see_hit_size", see_hit_size);
    writeArr("see_hitIdx", see_hitIdx);
    writeArr("see_hitType", see_hitType);
    writeArr("see_algo", see_algo);
    writeArr("ph2_detId", ph2_detId);
    writeArr("ph2_clustSize", ph2_clustSize);
    writeFloatArr("ph2_x", ph2_x);
    writeFloatArr("ph2_y", ph2_y);
    writeFloatArr("ph2_z", ph2_z);
    {
      char buf[32];
      std::snprintf(buf, sizeof(buf), "%g", static_cast<float>(ptCut_));
      std::string s(buf);
      if (s.find('.') == std::string::npos && s.find('e') == std::string::npos)
        s += ".0";
      ev << "      \"ptCut\": [" << s << "],\n";
    }
    ev << "      \"clustSizeCut\": [16]\n";
    ev << "    }";
    const std::string evStr = ev.str();

    // Read existing file (if any) to decide whether to create or append.
    std::ifstream existingFile("LST_input.json");
    if (!existingFile.good()) {
      // First event: create the file with the data array.
      std::ofstream out("LST_input.json");
      out << "{\n  \"data\": [\n" << evStr << "\n  ]\n}\n";
    } else {
      // Subsequent events: strip the closing ]} and append a new entry.
      std::ostringstream ss;
      ss << existingFile.rdbuf();
      existingFile.close();
      std::string content = ss.str();
      // Truncate just before the last ']' (which closes the "data" array).
      auto pos = content.rfind(']');
      if (pos != std::string::npos) content.resize(pos);
      std::ofstream out("LST_input.json");
      out << content << ",\n" << evStr << "\n  ]\n}\n";
    }
  }
  */
  // Claude Code's opinion
  // Resolve dynamic dims [-1] to actual runtime sizes before sending to Triton.
  // Without these calls the server rejects the request with "got [-1]".
  const int64_t n_seeds  = static_cast<int64_t>(see_px.size());
  const int64_t n_hits   = static_cast<int64_t>(see_hitIdx.size());
  const int64_t n_ph2    = static_cast<int64_t>(ph2_detId.size());

  input_see_px.setShape({n_seeds});
  input_see_py.setShape({n_seeds});
  input_see_pz.setShape({n_seeds});
  input_see_dxy.setShape({n_seeds});
  input_see_dz.setShape({n_seeds});
  input_see_ptErr.setShape({n_seeds});
  input_see_etaErr.setShape({n_seeds});
  input_see_stateTrajGlbX.setShape({n_seeds});
  input_see_stateTrajGlbY.setShape({n_seeds});
  input_see_stateTrajGlbZ.setShape({n_seeds});
  input_see_stateTrajGlbPx.setShape({n_seeds});
  input_see_stateTrajGlbPy.setShape({n_seeds});
  input_see_stateTrajGlbPz.setShape({n_seeds});
  input_see_q.setShape({n_seeds});
  input_see_hit_size.setShape({n_seeds});
  input_see_hitIdx.setShape({n_hits});
  input_see_hitType.setShape({n_hits});
  input_see_algo.setShape({n_seeds});
  input_ph2_detId.setShape({n_ph2});
  input_ph2_clustSize.setShape({n_ph2});
  input_ph2_x.setShape({n_ph2});
  input_ph2_y.setShape({n_ph2});
  input_ph2_z.setShape({n_ph2});
  // ptCut and clustSizeCut have fixed dims: [1] in config.pbtxt — no setShape needed

  input_see_px.toServer(inputdata_see_px);
  input_see_py.toServer(inputdata_see_py);
  input_see_pz.toServer(inputdata_see_pz);
  input_see_dxy.toServer(inputdata_see_dxy);
  input_see_dz.toServer(inputdata_see_dz);
  input_see_ptErr.toServer(inputdata_see_ptErr);
  input_see_etaErr.toServer(inputdata_see_etaErr);
  input_see_stateTrajGlbX.toServer(inputdata_see_stateTrajGlbX);
  input_see_stateTrajGlbY.toServer(inputdata_see_stateTrajGlbY);
  input_see_stateTrajGlbZ.toServer(inputdata_see_stateTrajGlbZ);
  input_see_stateTrajGlbPx.toServer(inputdata_see_stateTrajGlbPx);
  input_see_stateTrajGlbPy.toServer(inputdata_see_stateTrajGlbPy);
  input_see_stateTrajGlbPz.toServer(inputdata_see_stateTrajGlbPz);
  input_see_q.toServer(inputdata_see_q);
  input_see_hit_size.toServer(inputdata_see_hit_size);
  input_see_hitIdx.toServer(inputdata_see_hitIdx);
  input_see_hitType.toServer(inputdata_see_hitType);
  input_see_algo.toServer(inputdata_see_algo);
  input_ph2_detId.toServer(inputdata_ph2_detId);
  input_ph2_clustSize.toServer(inputdata_ph2_clustSize);
  input_ph2_x.toServer(inputdata_ph2_x);
  input_ph2_y.toServer(inputdata_ph2_y);
  input_ph2_z.toServer(inputdata_ph2_z);
  input_ptCut.toServer(inputdata_ptCut);
  input_clustSizeCut.toServer(inputdata_clustSizeCut);
}

void LSTSONICProducer::produce(edm::Event& iEvent, edm::EventSetup const& iSetup, Output const& iOutput) {

  // YY: break point
  std::cout << "YY: Start LSTSONICProducer::produce" << std::endl;

  // const auto &output = iOutput.begin()->second;
  // const auto &outputs_from_server = output.fromServer<uint8_t>();

  // Get output from server
  const auto &output_nTrackCandidates = iOutput.at("nTrackCandidates");
  const auto &nTrackCandidates_from_server = output_nTrackCandidates.fromServer<uint32_t>();
  auto nTrackCandidates_span = (nTrackCandidates_from_server[0]); // What does this [0] mean? and what is nTrackCandidate type here...
  //std::vector<unsigned int> nTrackCandidates(nTrackCandidates_span.begin(), nTrackCandidates_span.end());
  unsigned int nTrackCandidates(nTrackCandidates_span[0]);

  const auto &output_pixelSeedIndex = iOutput.at("pixelSeedIndex");
  const auto &pixelSeedIndex_from_server = output_pixelSeedIndex.fromServer<uint32_t>();
  auto pixelSeedIndex_span = (pixelSeedIndex_from_server[0]);
  std::vector<unsigned int> pixelSeedIndex(pixelSeedIndex_span.begin(), pixelSeedIndex_span.end());

  const auto &output_trackCandidateType = iOutput.at("trackCandidateType");
  const auto &trackCandidateType_from_server = output_trackCandidateType.fromServer<int8_t>();
  auto trackCandidateType_span = (trackCandidateType_from_server[0]);
  std::vector<int8_t> trackCandidateType(trackCandidateType_span.begin(), trackCandidateType_span.end());

  // YY: I might not need this any more... 
  /*
  const auto &output_nHits = iOutput.at("nHits");
  const auto &nHits_from_server = output_nHits.fromServer<uint16_t>();
  auto nHits_span = (nHits_from_server[0]);
  std::vector<unsigned int> nHits(nHits_span.begin(), nHits_span.end());
  */

  const auto &output_hitIndices = iOutput.at("hitIndices");
  const auto &hitIndices_from_server = output_hitIndices.fromServer<uint32_t>();
  auto hitIndices_span = (hitIndices_from_server[0]);
  std::vector<unsigned int> hitIndices_flat(hitIndices_span.begin(), hitIndices_span.end());

  /*
  std::vector <int> output_vec;
  int output_len = output.size()/sizeof(int);
  for (int i=0; i<output_len; i++) {
      int tmp = 0;
      std::memcpy(&tmp,&(output.front())+i*sizeof(int),sizeof(int));
      if (prints) { std::cout << "tmp: " << tmp << std::endl; };
      output_vec.push_back(tmp);
  };
  */

  /*
  std::vector<lst::Params_TC::ArrayUxHits> hitIndices;
  size_t index = 0;
  for (int size : nHits) {
      hitIndices.emplace_back(hitIndices_flat.begin() + index, hitIndices_flat.begin() + index + size); // Hope implicit converion works..
      index += size;
  }
  */

  // Convert hitIndices into 2D array based on Params_TC::ArrayUxHits structure
  lst::Params_TC::ArrayUxHits hitIndice{};
  hitIndice[0][0] = 1;
  std::vector<lst::Params_TC::ArrayUxHits> hitIndices;

  const unsigned int hits_per_track = lst::Params_TC::kLayers * lst::Params_TC::kHitsPerLayer;
  unsigned int n_tracks = hitIndices_flat.size() / hits_per_track; // YY: based on the definition in LSTCore/interface/Common.h
  hitIndices.reserve(n_tracks);  //  one allocation max. nHits.size() is number of tracks

  size_t index = 0;

  for (unsigned int track = 0; track < n_tracks; ++track) {

    lst::Params_TC::ArrayUxHits arr{};  // zero-initialized

    for (unsigned int layer = 0; layer < lst::Params_TC::kLayers; ++layer) {
      for (unsigned int hit = 0; hit < lst::Params_TC::kHitsPerLayer; ++hit) {
        arr[layer][hit] =
          hitIndices_flat[index + layer * lst::Params_TC::kHitsPerLayer + hit];
      }
    }

    index += hits_per_track;

    hitIndices.emplace_back(arr);
  }


  // Convert trackCandidateType into LSTObject // Maybe not needed

  /************************************************ 
  // Convert output to lstOutputHC (SOA) !! this needs to be done in the alpaka folder 

  lst::TrackCandidatesBaseHostCollection lstOutputHC(nTrackCandidates, std::queue); // May not work... if it needs an alpaka queue... 
  auto lstTrackCandidates = lstOutputHC.view<lst::TrackCandidatesBaseSoA>();
  std::memcpy(lstTrackCandidates.nTrackCandidates().data(), nTrackCandidates.data(), 1.0 * sizeof(unsigned int));
  std::memcpy(lstTrackCandidates.pixelSeedIndex().data(), pixelSeedIndex.data(), pixelSeedIndex.size() * sizeof(unsigned int));
  std::memcpy(lstTrackCandidates.trackCandidateType().data(), trackCandidateType.data(), trackCandidateType.size() * sizeof(short));
  std::memcpy(lstTrackCandidates.hitIndices().data(), hitIndices_2D.data(), hitIndices_2D.size() * sizeof(lst::Params_TC::ArrayUxHits)); // If does not work, use kHits * sizeof(unsigned int)

  // Output
  iEvent.emplace(lstOutputToken_, std::move(*lstTrackCandidates.release()));

  ************************************************/

  // Output: put nTrackCandidates, pixelSeedIndex, trackCandidateType, hitIndices into event.
  /*
  std::cout << "YY: Output to server: " << std::endl;
  std::cout << "nTrackCandidates: " << nTrackCandidates << std::endl;
  std::cout << "pixelSeedIndex : size "   << pixelSeedIndex.size()
            << " first element: " << pixelSeedIndex.front()
            << " last element "   << pixelSeedIndex.back() << std::endl;
  std::cout << "trackCandidateType : size "   << trackCandidateType.size()
            << " first element: " << trackCandidateType.front()
            << " last element "   << trackCandidateType.back() << std::endl;
  std::cout << "hitIndices : size "   << hitIndices.size() << std::endl;
  std::cout << " first element: " << std::endl;
  for (unsigned int layerSlot = lst::Params_TC::kPixelLayerSlots; layerSlot < lst::Params_TC::kLayers; ++layerSlot) {
    for (unsigned int hitSlot = 0; hitSlot < lst::Params_TC::kHitsPerLayer; ++hitSlot) {
      std::cout << hitIndices[0][layerSlot][hitSlot] << " ";
    }
  }
  std::cout << std::endl;
  std::cout << " last element: " << std::endl;
  for (unsigned int layerSlot = lst::Params_TC::kPixelLayerSlots; layerSlot < lst::Params_TC::kLayers; ++layerSlot) {
    for (unsigned int hitSlot = 0; hitSlot < lst::Params_TC::kHitsPerLayer; ++hitSlot) {
      std::cout << hitIndices[hitIndices.size()-1][layerSlot][hitSlot] << " ";
    }
  }  
  std::cout << std::endl;
  */

  iEvent.emplace(nTrackCandidatesPutToken_, nTrackCandidates);
  iEvent.emplace(pixelSeedIndexPutToken_, std::move(pixelSeedIndex));
  iEvent.emplace(trackCandidateTypePutToken_, std::move(trackCandidateType)); 
  iEvent.emplace(hitIndicesPutToken_, std::move(hitIndices)); 
}

void LSTSONICProducer::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
  edm::ParameterSetDescription desc;
  TritonClient::fillPSetDescription(desc);
  desc.add<double>("ptCut", 0.8);
  desc.add<edm::InputTag>("phase2OTRecHits", edm::InputTag("siPhase2RecHits"));
  desc.add<edm::InputTag>("beamSpot", edm::InputTag("offlineBeamSpot"));
  desc.add<std::vector<edm::InputTag>>("seedTracks",
                                        std::vector<edm::InputTag>{edm::InputTag("lstInitialStepSeedTracks"),
                                                                  edm::InputTag("lstHighPtTripletStepSeedTracks")});
  desc.add<bool>("verbose", false);
  desc.add<std::string>("ptCutLabel", "0.8");
  desc.add<bool>("nopLSDupClean", false);
  desc.add<bool>("tcpLSTriplets", false);
  descriptions.addWithDefaultLabel(desc);
}


DEFINE_FWK_MODULE(LSTSONICProducer); 
