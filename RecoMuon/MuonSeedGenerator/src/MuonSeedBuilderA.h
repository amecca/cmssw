#ifndef RecoMuon_MuonSeedBuilderA_H
#define RecoMuon_MuonSeedBuilderA_H

/** \class MuonSeedBuilderA
 *
 * Algorith to build TrajectorySeed for muon standalone reconstruction.
 * The segments are sorted out to make a protoTrack (vector of matching segments in different 
 * stations a.k.a. layers), for DT+overlap and CSC regions, in that order.  
 * The protoTrack is then passed to the seed creator to create CSC, overlap and/or DT seeds.
 *
 * Modified to use information global to build seed
 *
 * \author Shih-Chuan Kao, Dominique Fortin - UCR, Alberto Mecca - Torino
 *
 */

#include <FWCore/ParameterSet/interface/ParameterSet.h>
#include <RecoMuon/MeasurementDet/interface/MuonDetLayerMeasurements.h>
#include <DataFormats/TrajectorySeed/interface/TrajectorySeedCollection.h>
#include "FWCore/Framework/interface/ConsumesCollector.h"
#include "RecoMuon/TrackingTools/interface/MuonServiceProxy.h"

#include "DataFormats/MuonReco/interface/Muon.h"
#include "DataFormats/MuonReco/interface/MuonFwd.h"

#include <vector>

class DetLayer;
class MuonDetLayerGeometry;
class MagneticField;
class MuonSeedCreator;
class MuonSeedCleaner;

typedef std::vector<TrajectorySeed> SeedContainer;

class MuonSeedBuilderA {
public:
  typedef MuonTransientTrackingRecHit::MuonRecHitContainer SegmentContainer;
  typedef std::deque<bool> BoolContainer;

  /// Constructor
  explicit MuonSeedBuilderA(const edm::ParameterSet&, edm::ConsumesCollector&);

  /// Destructor
  ~MuonSeedBuilderA();

  // Operations

  /// Cache pointer to geometry
  void setGeometry(const MuonDetLayerGeometry* lgeom) { muonLayers = lgeom; }

  /// Cache pointer to Magnetic field
  void setBField(const MagneticField* theField) { BField = theField; }

  /// Build seed collection
  int build(edm::Event& event, const edm::EventSetup& eventSetup, const edm::Handle<reco::MuonCollection>& muons, TrajectorySeedCollection& seeds);

  std::vector<int> badSeedLayer;

private:
  /// cleaning the seeds
  // std::vector<TrajectorySeed> seedCleaner(const edm::EventSetup& eventSetup, std::vector<TrajectorySeed>& seeds);

  /// calculate the eta error from global R error
  double etaError(const GlobalPoint gp, double rErr);

  /// group the seeds
  //std::vector<SeedContainer> GroupSeeds( std::vector<TrajectorySeed>& seeds );
  /// pick the seed by better parameter error
  //TrajectorySeed BetterDirection( std::vector<TrajectorySeed>& seeds ) ;
  //TrajectorySeed BetterChi2( std::vector<TrajectorySeed>& seeds );
  /// filter out the bad pt seeds, if all are bad pt seeds then keep all
  //bool MomentumFilter(std::vector<TrajectorySeed>& seeds );

  /// collect long seeds
  //SeedContainer LengthFilter(std::vector<TrajectorySeed>& seeds );
  /// pick the seeds w/ 1st layer information and w/ more than 1 segments
  //SeedContainer SeedCandidates( std::vector<TrajectorySeed>& seeds, bool good );
  /// check overlapping segment for seeds
  //unsigned int OverlapSegments( TrajectorySeed seed1, TrajectorySeed seed2 );

  /// retrieve number of rechits& normalized chi2 of associated segments of a seed
  //int NRecHitsFromSegment( const TrackingRecHit& rhit );
  //int NRecHitsFromSegment( MuonTransientTrackingRecHit *rhit );
  //int NRecHitsFromSegment( const MuonTransientTrackingRecHit& rhit );
  //double NChi2OfSegment( const TrackingRecHit& rhit );

  /// retrieve seed global position
  //GlobalPoint SeedPosition( TrajectorySeed seed );
  /// retrieve seed global momentum
  //GlobalVector SeedMomentum( TrajectorySeed seed );

  // This Producer private debug flag
  bool debug;

  // Number of Segments from a shower
  int NShowerSeg;
  SegmentContainer ShoweringSegments;
  std::vector<int> ShoweringLayers;
  
  /// Create seed according to region (CSC, DT, Overlap)
  // MuonSeedCreator* muonSeedCreate_;
  MuonSeedCleaner* muonSeedClean_;

  // Service for propagator
  std::unique_ptr<MuonServiceProxy> theService;
  std::string propagatorName;

  // Cache geometry for current event
  const MuonDetLayerGeometry* muonLayers;

  // Cache Magnetic Field for current event
  const MagneticField* BField;

  // Minimum separation when we can distinguish between 2 muon seeds
  // (to suppress combinatorics)
  float maxEtaResolutionDT;
  float maxEtaResolutionCSC;
  float maxPhiResolutionDT;
  float maxPhiResolutionCSC;
  float theMinMomentum;
  
  float scaleInnerStateError;
};
#endif
