/**
 *  See header file for a description of this class.
 *  
 *  \author Shih-Chuan Kao, Dominique Fortin - UCR, Alberto Mecca - Torino
 */

#include <RecoMuon/MuonSeedGenerator/src/MuonSeedBuilderA.h>
#include <RecoMuon/MuonSeedGenerator/src/MuonSeedCreator.h>
#include <RecoMuon/MuonSeedGenerator/src/MuonSeedCleaner.h>

// Data Formats
#include <DataFormats/TrajectorySeed/interface/TrajectorySeedCollection.h>
#include <DataFormats/TrajectorySeed/interface/TrajectorySeed.h>
#include <DataFormats/CSCRecHit/interface/CSCRecHit2DCollection.h>
#include <DataFormats/CSCRecHit/interface/CSCRecHit2D.h>
#include <DataFormats/CSCRecHit/interface/CSCSegmentCollection.h>
#include <DataFormats/CSCRecHit/interface/CSCSegment.h>
#include <DataFormats/DTRecHit/interface/DTRecSegment4DCollection.h>
#include <DataFormats/DTRecHit/interface/DTRecSegment4D.h>

#include "DataFormats/TrackReco/interface/TrackFwd.h"
#include "DataFormats/TrackReco/interface/Track.h"

// Geometry
#include <Geometry/CommonDetUnit/interface/GeomDet.h>
#include <TrackingTools/DetLayers/interface/DetLayer.h>
#include <RecoMuon/MeasurementDet/interface/MuonDetLayerMeasurements.h>
#include <RecoMuon/DetLayers/interface/MuonDetLayerGeometry.h>
#include <RecoMuon/Records/interface/MuonRecoGeometryRecord.h>

// muon service
#include <TrackingTools/TrajectoryState/interface/TrajectoryStateTransform.h>
#include <DataFormats/TrajectoryState/interface/PTrajectoryStateOnDet.h>

// Framework
#include <FWCore/ParameterSet/interface/ParameterSet.h>
#include "FWCore/Framework/interface/Event.h"
#include <FWCore/Framework/interface/ESHandle.h>
#include <FWCore/MessageLogger/interface/MessageLogger.h>
#include <DataFormats/Common/interface/Handle.h>

#include "FWCore/Utilities/interface/InputTag.h"
#include "FWCore/Framework/interface/Frameworkfwd.h"

// C++
#include <vector>
#include <deque>
#include <utility>

//typedef std::pair<double, TrajectorySeed> seedpr ;
//static bool ptDecreasing(const seedpr s1, const seedpr s2) { return ( s1.first > s2.first ); }
//static bool lengthSorting(const TrajectorySeed s1, const TrajectorySeed s2) { return ( s1.nHits() > s2.nHits() ); }

/*
 * Constructor
 */
MuonSeedBuilderA::MuonSeedBuilderA(const edm::ParameterSet& pset, edm::ConsumesCollector& iC){
  // Local Debug flag
  debug = pset.getParameter<bool>("DebugMuonSeed");
  scaleInnerStateError = pset.exists("scaleInnerStateError") ? pset.getParameter<double>("scaleInnerStateError") : 1.;
  edm::LogInfo("MuonSeedBuilderA") << "Building MuonSeedBuilderA with scaleInnerStateError = " << scaleInnerStateError;
  
  edm::ParameterSet serviceParameters = pset.getParameter<edm::ParameterSet>("ServiceParameters");
  propagatorName = serviceParameters.getParameter<std::string>("Propagator");
  serviceParameters.addUntrackedParameter("Propagators", std::vector<std::string>({propagatorName}));  // It wants a list of propagators to load
  theService = std::make_unique<MuonServiceProxy>(serviceParameters, edm::ConsumesCollector(iC));  // edm::ConsumesCollector());
  // thePatternRecognition->setServiceProxy(theService);
  
  // Class for forming muon seeds
  // muonSeedCreate_ = new MuonSeedCreator(pset);
  muonSeedClean_ = new MuonSeedCleaner(pset, std::move(iC));
}

/*
 * Destructor
 */
MuonSeedBuilderA::~MuonSeedBuilderA() {
  // delete muonSeedCreate_;
  delete muonSeedClean_;
}

/* 
 * build
 *
 * Where the segments are sorted out to make a protoTrack (vector of matching segments in different 
 * stations/layers).  The protoTrack is then passed to the seed creator to create CSC, DT or overlap seeds
 *
 */
int MuonSeedBuilderA::build(edm::Event& event, const edm::EventSetup& eventSetup, const edm::Handle<reco::MuonCollection>& muons, TrajectorySeedCollection& theSeeds){
  // edm::LogInfo("MuonSeedBuilderA") << "Start build()";
  // Pass the Magnetic Field to where the seed is create
  // muonSeedCreate_->setBField(BField);
  std::ostringstream eventInfo;  // collect information for a single LogInfo at the end of this function
  
  // Update the service
  theService->update(eventSetup);
  edm::ESHandle<MagneticField>          magneticField    = theService->magneticField();
  edm::ESHandle<GlobalTrackingGeometry> trackingGeometry = theService->trackingGeometry();
  
  // Get the propagator from the (updated) service
  const edm::ESHandle<Propagator> propagator = theService->propagator(propagatorName);
  if(! propagator.isValid()){
    edm::LogWarning("MuonSeedBuilderA") << "Propagator is invalid";
    return -1;  // This should be interpreted as an error code. I hope.
  }
  
  // Create temporary collection of seeds which will be cleaned up to remove combinatorics
  std::vector<TrajectorySeed> seedsFromAllSdAHits, seedsFromValidSdAHits;
  std::vector<float> etaOfSeed;
  std::vector<float> phiOfSeed;
  std::vector<int> nSegOnSeed;
  
  size_t nMuonsWithInner = 0;
  for(const reco::Muon& muon : *muons){
    reco::TrackRef inner = muon.innerTrack();
    if(inner.isNull()){
      eventInfo << "Discarding muon with null track\n";
      continue;
    }
    // TODO update with same requirement as tracker muons
    if(! (inner->chi2() / inner->ndof() <= 2.5 && inner->numberOfValidHits() >= 5 && inner->hitPattern().numberOfValidPixelHits() >= 2 && inner->quality(reco::TrackBase::highPurity)) ){
      eventInfo << "Discarding track with: chi2/ndof = "<<inner->chi2()/inner->ndof()<<"  valid_hits = "<<inner->numberOfValidHits()
	        << "  inner_valid_hits = "<<inner->hitPattern().numberOfValidPixelHits()<<"  quality_mask = "<<inner->qualityMask() << '\n';
      continue;
    }
    
    nMuonsWithInner++;
    
    // get information about the outermost tracker hit
    GlobalPoint outerPosition(inner->outerPosition().x(),
    			      inner->outerPosition().y(),
    			      inner->outerPosition().z());
    GlobalVector outerMomentum(inner->outerMomentum().x(),
    			       inner->outerMomentum().y(),
    			       inner->outerMomentum().z());
    int charge = inner->charge();
    const reco::Track::CovarianceMatrix outerStateCovariance = inner->outerStateCovariance();
    DetId trackerOuterDetID = DetId(inner->outerDetId());
    
    // construct the information necessary to make a TrajectoryStateOnSurface
    GlobalTrajectoryParameters globalTrajParams(outerPosition, outerMomentum, charge, magneticField.product());
    CurvilinearTrajectoryError curviError(outerStateCovariance);
    
    // starting point for propagation into the muon system
    TrajectoryStateOnSurface tracker_tsos = TrajectoryStateOnSurface(globalTrajParams, curviError, trackingGeometry->idToDet(trackerOuterDetID)->surface());
    if(! tracker_tsos.isValid()){
      eventInfo << "Invlid tracker TSOS!";
      continue;
    }
    
    // propagate the TSOS to muon system. Which one?
    
    // Create seed
    //   1. with same segment(s) as the SdA, if present --> resolution increase?
    //      - from innermost track, deduce where to propagate the inner track
    //      - only segments from DTs and CSCs to the seed! Use track.recHits().begin()/end() to iterate and select the segments
    //      - alternatively, get track.seed() and feed its segments to the pattern recognition; only initial state from propagated inner
    //   2. look for segments "nearby": exaustveTrajectoryBuilder --> efficiency increase?
    //   3. if no segments, use standAloneTrajectoryBuilder --> efficiency increase?
    
    reco::TrackRef outer = muon.outerTrack();
    if(! outer.isNull() ){  // We already have a SdA: use it at least to choose where to propagate the state
      DetId sdaInnerDetID = DetId(outer->innerDetId());  // the INNERmost DetId of the outer (SdA) track
      TrajectoryStateOnSurface propagated_tsos = propagator->propagate(tracker_tsos, trackingGeometry->idToDet(sdaInnerDetID)->surface());
      eventInfo << "Extrapolated from " << std::hex << trackerOuterDetID() << " to " << sdaInnerDetID() << " - Status: " << (propagated_tsos.isValid() ?"success":"invalid") << std::dec << '\t';
      propagated_tsos.rescaleError(scaleInnerStateError);
      
      // Use the hits from the outer track to build seed, but with the initial state extrapolated from the inner track
      if(! outer->recHits().empty()){
    	PTrajectoryStateOnDet const& PTraj = trajectoryStateTransform::persistentState(propagated_tsos, sdaInnerDetID.rawId());
	
    	edm::OwnVector<TrackingRecHit> allSdAHits, validSdAHits;
    	for(const auto& hit : outer->recHits()){
    	  DetId id = hit->geographicalId();
    	  if(! (id.det() == DetId::Muon && (id.subdetId() == MuonSubdetId::DT || id.subdetId() == MuonSubdetId::CSC)) )
	    continue;  // Use only segments from DT/CSC to build the seeds

	  allSdAHits.push_back(hit->clone());
	  
    	  TrajectoryStateOnSurface extrapolation = propagator->propagate(tracker_tsos, trackingGeometry->idToDet(id)->surface());
	  if(extrapolation.isValid())
	    validSdAHits.push_back(hit->clone());
	  else
	    eventInfo << "\tPropagation of inner track to outer hit failed.\n";
	}
	
	TrajectorySeed trajectorySeedAllSdA  (PTraj, allSdAHits, PropagationDirection::alongMomentum);
	seedsFromAllSdAHits.push_back  (trajectorySeedAllSdA);	
	// TrajectorySeed trajectorySeedValidSdA(PTraj, allSdAHits, PropagationDirection::alongMomentum);
	// seedsFromValidSdAHits.push_back(trajectorySeedValidSdA);
	
	eventInfo << "DT/CSC hits from SdA: " << allSdAHits.size() << ( validSdAHits.size()==allSdAHits.size() ? "\n" : Form(" (valid: %zu)\n",validSdAHits.size()) );
      }
    }
    
    else{ // outer.isNull()
      eventInfo << "This muon has no outer track.\n";
      // TODO: in this case, run MuonSeedBuilder (standard) to re-create all the seeds
      //for(const DetLayer* pDetLayer : trackingGeometry->allDTLayers())
      //for(const auto& hit :  )
    }

  } // end loop on muons
  
  if(muons->size() != nMuonsWithInner)
    eventInfo << "Muons: " << muons->size()<<" - Muons with inner track: "<<nMuonsWithInner << '\n';
  else
    eventInfo << "All " << nMuonsWithInner << " muons accepted.\n";
  
  // Instantiate the accessor (get the segments: DT + CSC but not RPC=false)
  // MuonDetLayerMeasurements muonMeasurements(enableDTMeasurement,enableCSCMeasurement,false,
  //                                          theDTSegmentLabel.label(),theCSCSegmentLabel.label());

  // // 1) Get the various stations and store segments in containers for each station (layers)

  // // 1a. get the DT segments by stations (layers):
  // std::vector<const DetLayer*> dtLayers = muonLayers->allDTLayers();

  // SegmentContainer DTlist4 = muonMeasurements->recHits(dtLayers[3], event);
  // SegmentContainer DTlist3 = muonMeasurements->recHits(dtLayers[2], event);
  // SegmentContainer DTlist2 = muonMeasurements->recHits(dtLayers[1], event);
  // SegmentContainer DTlist1 = muonMeasurements->recHits(dtLayers[0], event);

  // // Initialize flags that a given segment has been allocated to a seed
  // BoolContainer usedDTlist4(DTlist4.size(), false);
  // BoolContainer usedDTlist3(DTlist3.size(), false);
  // BoolContainer usedDTlist2(DTlist2.size(), false);
  // BoolContainer usedDTlist1(DTlist1.size(), false);

  // if (debug) {
  //   std::cout << "*** Number of DT segments is: " << DTlist4.size() + DTlist3.size() + DTlist2.size() + DTlist1.size()
  //             << std::endl;
  //   std::cout << "In MB1: " << DTlist1.size() << std::endl;
  //   std::cout << "In MB2: " << DTlist2.size() << std::endl;
  //   std::cout << "In MB3: " << DTlist3.size() << std::endl;
  //   std::cout << "In MB4: " << DTlist4.size() << std::endl;
  // }

  // // 1b. get the CSC segments by stations (layers):
  // // 1b.1 Global z < 0
  // std::vector<const DetLayer*> cscBackwardLayers = muonLayers->backwardCSCLayers();
  // SegmentContainer CSClist4B = muonMeasurements->recHits(cscBackwardLayers[4], event);
  // SegmentContainer CSClist3B = muonMeasurements->recHits(cscBackwardLayers[3], event);
  // SegmentContainer CSClist2B = muonMeasurements->recHits(cscBackwardLayers[2], event);
  // SegmentContainer CSClist1B = muonMeasurements->recHits(cscBackwardLayers[1], event);  // ME1/2 and 1/3
  // SegmentContainer CSClist0B = muonMeasurements->recHits(cscBackwardLayers[0], event);  // ME11

  // BoolContainer usedCSClist4B(CSClist4B.size(), false);
  // BoolContainer usedCSClist3B(CSClist3B.size(), false);
  // BoolContainer usedCSClist2B(CSClist2B.size(), false);
  // BoolContainer usedCSClist1B(CSClist1B.size(), false);
  // BoolContainer usedCSClist0B(CSClist0B.size(), false);

  // // 1b.2 Global z > 0
  // std::vector<const DetLayer*> cscForwardLayers = muonLayers->forwardCSCLayers();
  // SegmentContainer CSClist4F = muonMeasurements->recHits(cscForwardLayers[4], event);
  // SegmentContainer CSClist3F = muonMeasurements->recHits(cscForwardLayers[3], event);
  // SegmentContainer CSClist2F = muonMeasurements->recHits(cscForwardLayers[2], event);
  // SegmentContainer CSClist1F = muonMeasurements->recHits(cscForwardLayers[1], event);
  // SegmentContainer CSClist0F = muonMeasurements->recHits(cscForwardLayers[0], event);

  // BoolContainer usedCSClist4F(CSClist4F.size(), false);
  // BoolContainer usedCSClist3F(CSClist3F.size(), false);
  // BoolContainer usedCSClist2F(CSClist2F.size(), false);
  // BoolContainer usedCSClist1F(CSClist1F.size(), false);
  // BoolContainer usedCSClist0F(CSClist0F.size(), false);

  // if (debug) {
  //   std::cout << "*** Number of CSC segments is "
  //             << CSClist4F.size() + CSClist3F.size() + CSClist2F.size() + CSClist1F.size() + CSClist0F.size() +
  //                    CSClist4B.size() + CSClist3B.size() + CSClist2B.size() + CSClist1B.size() + CSClist0B.size()
  //             << std::endl;
  //   std::cout << "In ME11: " << CSClist0F.size() + CSClist0B.size() << std::endl;
  //   std::cout << "In ME12: " << CSClist1F.size() + CSClist1B.size() << std::endl;
  //   std::cout << "In ME2 : " << CSClist2F.size() + CSClist2B.size() << std::endl;
  //   std::cout << "In ME3 : " << CSClist3F.size() + CSClist3B.size() << std::endl;
  //   std::cout << "In ME4 : " << CSClist4F.size() + CSClist4B.size() << std::endl;
  // }
  
  

  /* ******************************************************************************************************************
   * Form seeds in barrel region
   *
   * Proceed from inside -> out
   * ******************************************************************************************************************/

  // Loop over all possible MB1 segment to form seeds:
  // int index = -1;
  // for (SegmentContainer::iterator it = DTlist1.begin(); it != DTlist1.end(); ++it) {
    // TrajectorySeed thisSeed;
    // thisSeed = muonSeedCreate_->createSeed(3, protoTrack, layers, NShowers, NShowerSeg);
    
    // Add the seeds to master collection
    // rawSeeds.push_back(thisSeed);
    // etaOfSeed.push_back(eta_temp);
    // phiOfSeed.push_back(phi_temp);
    // nSegOnSeed.push_back(protoTrack.size());

    // Marked segment as used
    // usedDTlist1[index] = true;
  // }

  // Loop over all possible MB2 segment to form seeds:
  // Loop over all possible MB3 segment to form seeds:
  
  /* *********************************************************************************************************************
   * Form seeds from backward endcap
   *
   * Proceed from inside -> out
   * *********************************************************************************************************************/

  // Loop over all possible ME11 segment to form seeds:
  // Loop over all possible ME1/2 ME1/3 segment to form seeds:
  // Loop over all possible ME2 segment to form seeds:
  // Loop over all possible ME3 segment to form seeds:

  /* *****************************************************************************************************************
   * Form seeds from forward endcap
   *
   * Proceed from inside -> out
   * *****************************************************************************************************************/

  // Loop over all possible ME11 segment to form seeds:
  // Loop over all possible ME2 segment to form seeds:

  /* *********************************************************************************************************************
   * Clean up raw seed collection and pass to master collection
   * *********************************************************************************************************************/

  int NGoodSeeds = 0;

  theSeeds = muonSeedClean_->seedCleaner(eventSetup, seedsFromAllSdAHits);
  NGoodSeeds = theSeeds.size();
  
  eventInfo << " === Before cleaning: " << seedsFromAllSdAHits.size() << " => After: " << NGoodSeeds << '\n';

  std::string eventInfoStr = eventInfo.str();
  eventInfoStr.pop_back();
  edm::LogInfo("MuonSeedBuilderA") << eventInfoStr;
  // edm::LogInfo("MuonSeedBuilderA") << "End build()";
  return NGoodSeeds;
}

/* *********************************************************************************************************************
 * Try to match segment from different station (layer)
 *
 * Note type = 1 --> endcap
 *           = 2 --> overlap
 *           = 3 --> barrel
 * *********************************************************************************************************************/

///                                                   segment for seeding         , segments collection
