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


TrajectorySeed seedFromSdA    (const reco::Muon&, const TrajectoryStateOnSurface&, const edm::ESHandle<Propagator>&, const edm::ESHandle<GlobalTrackingGeometry>&, double scaleInnerStateError);
TrajectorySeed seedFromTracker(const reco::Muon&, const TrajectoryStateOnSurface&, const edm::ESHandle<Propagator>&, const edm::ESHandle<GlobalTrackingGeometry>&, double scaleInnerStateError);

/* 
 * build
 *
 * Where the segments are sorted out to make a protoTrack (vector of matching segments in different 
 * stations/layers).  The protoTrack is then passed to the seed creator to create CSC, DT or overlap seeds
 *
 */
int MuonSeedBuilderA::build(edm::Event& event, const edm::EventSetup& eventSetup, const edm::Handle<reco::MuonCollection>& muons, TrajectorySeedCollection& theSeeds){
  // edm::LogInfo("MuonSeedBuilderA") << "Start build()";
  
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
  std::vector<TrajectorySeed> seedsFromValidSdAHits; //seedsFromAllSdAHits
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
    
    if(debug){
      if(muon.outerTrack().isAvailable()){
	std::cout<<"Muon #"<<std::distance(&muon, &*muons->begin())<<" - ";
	edm::RefToBase<TrajectorySeed> seedRef_outer = muon.outerTrack()->seedRef();
	if(! seedRef_outer.isAvailable()){
	  std::cout<<"seeRef NOT available\n";
	}
	else{
	  std::cout<<seedRef_outer.get()<<'\n';
	}
      }
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
    
    TrajectorySeed seedSdA     =     seedFromSdA(muon, tracker_tsos, propagator, trackingGeometry, scaleInnerStateError);
    if(seedSdA.nHits() > 0){
      eventInfo << "Made seed from SdA hits.\n";
      seedsFromValidSdAHits.push_back(seedSdA);
      continue;
    }
    TrajectorySeed seedTracker = seedFromTracker(muon, tracker_tsos, propagator, trackingGeometry, scaleInnerStateError);
    if(seedTracker.nHits() > 0){
      eventInfo << "Made seed from segments associated to inner track.\n";
      seedsFromValidSdAHits.push_back(seedTracker);
      continue;
    }
    eventInfo << "No hits associated, resort to standAloneTrajectoryBuilder.\n";

  } // end loop on muons
  
  size_t NGoodSeeds = seedsFromValidSdAHits.size();
  eventInfo << "Seeds/Muons with inner/total: " << NGoodSeeds << '/' << nMuonsWithInner << '/' << muons->size() << '\n';
  
  // IMPORTANT: fill the output collection
  theSeeds = std::move(seedsFromValidSdAHits);
  
  std::string eventInfoStr = eventInfo.str();
  eventInfoStr.pop_back();  // remove last '\n'
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

TrajectorySeed seedFromSdA(const reco::Muon& muon, const TrajectoryStateOnSurface& tracker_tsos, const edm::ESHandle<Propagator>& propagator, const edm::ESHandle<GlobalTrackingGeometry>& trackingGeometry, double scaleInnerStateError){
  reco::TrackRef outer = muon.outerTrack();
  if(outer.isNull())
    return TrajectorySeed();
  
  // We already have a SdA: use it at least to choose where to propagate the state
  DetId sdaInnerDetID = DetId(outer->innerDetId());  // the INNERmost DetId of the outer (SdA) track
  TrajectoryStateOnSurface propagated_tsos = propagator->propagate(tracker_tsos, trackingGeometry->idToDet(sdaInnerDetID)->surface());
  // eventInfo << "Extrapolated from " << std::hex << trackerOuterDetID() << " to " << sdaInnerDetID() << " - Status: " << (propagated_tsos.isValid() ?"success":"invalid") << std::dec << '\t';
  propagated_tsos.rescaleError(scaleInnerStateError);
  
  // Use the hits from the outer track to build seed, but with the initial state extrapolated from the inner track
  if(outer->recHits().empty())
    return TrajectorySeed();
  
  PTrajectoryStateOnDet const& PTraj = trajectoryStateTransform::persistentState(propagated_tsos, sdaInnerDetID.rawId());
  
  edm::OwnVector<TrackingRecHit> validSdAHits; //allSdAHits
  for(const auto& hit : outer->recHits()){
    DetId id = hit->geographicalId();
    if(! (id.det() == DetId::Muon && (id.subdetId() == MuonSubdetId::DT || id.subdetId() == MuonSubdetId::CSC)) )
      continue;  // Use only segments from DT/CSC to build the seeds
    // if(! hit->isValid())
    //   continue;  // Exclude invalid hits since they may lack some information and make the pattern recognition crash
    
    validSdAHits.push_back(hit->clone());
  }
  
  return TrajectorySeed(PTraj, validSdAHits, PropagationDirection::alongMomentum);
}


TrajectorySeed seedFromTracker(const reco::Muon& muon, const TrajectoryStateOnSurface& tracker_tsos, const edm::ESHandle<Propagator>& propagator, const edm::ESHandle<GlobalTrackingGeometry>& trackingGeometry, double scaleInnerStateError){
  TrajectoryStateOnSurface propagated_tsos;
  DetId propagatedChamberID;
  bool propagatedToChambers = false;
  edm::OwnVector<TrackingRecHit> hitsForSeed;

  for(const reco::MuonChamberMatch& match : muon.matches()){
    DetId chamberID = match.id;
    bool isDT  = chamberID.subdetId() == MuonSubdetId::DT;
    bool isCSC = chamberID.subdetId() == MuonSubdetId::CSC;
    
    for(const reco::MuonSegmentMatch& segmentMatch : match.segmentMatches){
      if(isDT)
	hitsForSeed.push_back(segmentMatch.dtSegmentRef->clone());
      else if(isCSC)
	hitsForSeed.push_back(segmentMatch.cscSegmentRef->clone());
    }
	
    if( ! propagatedToChambers){
      TrajectoryStateOnSurface extrapolation = propagator->propagate(tracker_tsos, trackingGeometry->idToDet(chamberID)->surface());
      // eventInfo << "Extrapolated from " << std::hex << trackerOuterDetID() << " to " << chamberID() << " - Status: " << (propagated_tsos.isValid() ?"success":"invalid") << std::dec << '\n';
      if(extrapolation.isValid()){
	extrapolation.rescaleError(scaleInnerStateError);
	propagated_tsos = std::move(extrapolation);
	propagatedChamberID = chamberID;
	propagatedToChambers = true;
      }
    }
  }  // end loop on MuonChamberMatch
  
  if(propagatedToChambers && hitsForSeed.size() > 0){
    PTrajectoryStateOnDet const& PTraj = trajectoryStateTransform::persistentState(propagated_tsos, propagatedChamberID.rawId());
    return TrajectorySeed(PTraj, hitsForSeed, PropagationDirection::alongMomentum);
  }
  else
    return TrajectorySeed();
}
