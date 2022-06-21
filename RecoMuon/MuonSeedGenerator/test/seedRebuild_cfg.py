# Auto generated configuration file
# using: 
# Revision: 1.19 
# Source: /local/reps/CMSSW/CMSSW/Configuration/Applications/python/ConfigBuilder.py,v 
# with command line options: seedRebuild --step RECO --datatier RECO --eventcontent RECOSIM --filein=/store/relval/CMSSW_12_4_0_pre2/RelValSingleMuPt100/GEN-SIM-RECO/123X_mcRun3_2021_realistic_v12_TEST_alma-v2/10000/b08a880d-c9b8-4a64-83f6-33b15658a0db.root --conditions auto:run3_mc_FULL --geometry DB:Extended --era Run3 --scenario pp --number=-1 --no_exec
import FWCore.ParameterSet.Config as cms

from Configuration.Eras.Era_Run3_cff import Run3

from RecoMuon.TrackingTools.MuonServiceProxy_cff import *

process = cms.Process('RESTA',Run3)

# import of standard configurations
process.load('Configuration.StandardSequences.Services_cff')
process.load('SimGeneral.HepPDTESSource.pythiapdt_cfi')
process.load('FWCore.MessageService.MessageLogger_cfi')
process.load('Configuration.EventContent.EventContent_cff')
process.load('SimGeneral.MixingModule.mixNoPU_cfi')
process.load('Configuration.StandardSequences.GeometryRecoDB_cff')
process.load('Configuration.StandardSequences.MagneticField_cff')
process.load('Configuration.StandardSequences.Reconstruction_cff')
process.load('Configuration.StandardSequences.EndOfProcess_cff')
process.load('Configuration.StandardSequences.FrontierConditions_GlobalTag_cff')
# process.load('RecoMuon.Configuration.RecoMuonPPonly_cff')
process.load('RecoMuon.StandAloneMuonProducer.standAloneMuons_cff')
# process.load('RecoMuon.TrackingTools.MuonServiceProxy_cff')

process.MessageLogger = cms.Service("MessageLogger",
                        destinations   = cms.untracked.vstring(
                            'info'
                            ,'cout'
                        ),
                        categories = cms.untracked.vstring('MuonSeedBuilderA'),
                        info = cms.untracked.PSet(
                            threshold = cms.untracked.string('INFO'),
                            default          = cms.untracked.PSet( limit = cms.untracked.int32(0) ),
                            MuonSeedBuilderA = cms.untracked.PSet( limit = cms.untracked.int32(200) )
                        ),
                        cout = cms.untracked.PSet(
                            threshold = cms.untracked.string('WARNING') 
                        )
)

process.maxEvents = cms.untracked.PSet(
    input = cms.untracked.int32(-1),
    output = cms.untracked.int32(50),#cms.optional.untracked.allowed(cms.int32,cms.PSet)
)

# Input source
process.source = cms.Source("PoolSource",
    fileNames = cms.untracked.vstring('/store/relval/CMSSW_12_4_0_pre2/RelValSingleMuPt100/GEN-SIM-RECO/123X_mcRun3_2021_realistic_v12_TEST_alma-v2/10000/b08a880d-c9b8-4a64-83f6-33b15658a0db.root'),
    secondaryFileNames = cms.untracked.vstring()
)

process.options = cms.untracked.PSet(
    FailPath = cms.untracked.vstring(),
    IgnoreCompletely = cms.untracked.vstring(),
    Rethrow = cms.untracked.vstring(),
    SkipEvent = cms.untracked.vstring(),
    accelerators = cms.untracked.vstring('*'),
    allowUnscheduled = cms.obsolete.untracked.bool,
    canDeleteEarly = cms.untracked.vstring(),
    deleteNonConsumedUnscheduledModules = cms.untracked.bool(True),
    dumpOptions = cms.untracked.bool(False),
    emptyRunLumiMode = cms.obsolete.untracked.string,
    eventSetup = cms.untracked.PSet(
        forceNumberOfConcurrentIOVs = cms.untracked.PSet(
            allowAnyLabel_=cms.required.untracked.uint32
        ),
        numberOfConcurrentIOVs = cms.untracked.uint32(0)
    ),
    fileMode = cms.untracked.string('FULLMERGE'),
    forceEventSetupCacheClearOnNewRun = cms.untracked.bool(False),
    makeTriggerResults = cms.obsolete.untracked.bool,
    numberOfConcurrentLuminosityBlocks = cms.untracked.uint32(0),
    numberOfConcurrentRuns = cms.untracked.uint32(1),
    numberOfStreams = cms.untracked.uint32(0),
    numberOfThreads = cms.untracked.uint32(1),
    printDependencies = cms.untracked.bool(False),
    sizeOfStackForThreadsInKB = cms.optional.untracked.uint32,
    throwIfIllegalParameter = cms.untracked.bool(True),
    wantSummary = cms.untracked.bool(False)
)

# Production Info
process.configurationMetadata = cms.untracked.PSet(
    annotation = cms.untracked.string('seedRebuild nevts:-1'),
    name = cms.untracked.string('Applications'),
    version = cms.untracked.string('$Revision: 1.19 $')
)

# Modules
# process.rebuildSeeds = cms.EDProducer("MuonSeedProducer",
#                                       muons                = cms.InputTag("globalMuons"),
#                                       DebugMuonSeed        = cms.bool(False),
#                                       EnableDTMeasurement  = cms.bool(False),
#                                       DTSegmentLabel       = cms.InputTag("dt4DSegments"),
#                                       EnableCSCMeasurement = cms.bool(False),
#                                       CSCSegmentLabel      = cms.InputTag("cscSegments"),
#                                       minCSCHitsPerSegment = cms.int32(0),
#                                       maxDeltaEtaCSC      = cms.double(0.),
#                                       maxDeltaPhiCSC      = cms.double(0.),
#                                       maxDeltaEtaOverlap  = cms.double(0.),
#                                       maxDeltaPhiOverlap  = cms.double(0.),
#                                       minDTHitsPerSegment = cms.int32(0),
#                                       maxDeltaEtaDT       = cms.double(0.),
#                                       maxDeltaPhiDT       = cms.double(0.),
#                                       maxEtaResolutionDT  = cms.double(0.),
#                                       maxPhiResolutionDT  = cms.double(0.),
#                                       maxEtaResolutionCSC = cms.double(0.),
#                                       maxPhiResolutionCSC = cms.double(0.),
                                      
#                                       minimumSeedPt       = cms.double(0.),
#                                       maximumSeedPt       = cms.double(0.),
#                                       defaultSeedPt       = cms.double(0.)                                      
# )

process.seedFromGlobalMuons = cms.EDProducer("MuonSeedProducer",
                                             muons                = cms.InputTag("muons"),
                                             DebugMuonSeed        = cms.bool(False),
                                             scaleInnerStateError = cms.double(10.),
                                             ServiceParameters = cms.PSet(
                                                 Propagator = cms.string('SteppingHelixPropagatorAlong'),  # To test if this causes failures
                                                 RPCLayers  = cms.bool(True),
                                                 CSCLayers  = cms.bool(True),
                                                 GEMLayers  = cms.bool(True),
                                                 ME0Layers  = cms.bool(True)
                                             ),
                                             EnableDTMeasurement  = cms.bool(True),
                                             DTSegmentLabel       = cms.InputTag("dt4DSegments"),
                                             EnableCSCMeasurement = cms.bool(True),
                                             CSCSegmentLabel      = cms.InputTag("cscSegments"),
                                         )

process.standAloneMuons.InputObjects = "seedFromGlobalMuons"

# process.refittedStandAloneMuons = process.standAloneMuons.clone()

# Output definition

process.RECOSIMoutput = cms.OutputModule("PoolOutputModule",
    dataset = cms.untracked.PSet(
        dataTier = cms.untracked.string('RECO'),
        filterName = cms.untracked.string('')
    ),
    fileName = cms.untracked.string('seedRebuild_RECO.root'),
    outputCommands = process.RECOSIMEventContent.outputCommands,
    splitLevel = cms.untracked.int32(0)
)

process.RECOSIMoutput.outputCommands.append('keep *_seedFromGlobalMuons_*_*')

# Additional output definition

# Other statements
from Configuration.AlCa.GlobalTag import GlobalTag
process.GlobalTag = GlobalTag(process.GlobalTag, 'auto:run3_mc_FULL', '')

# Path and EndPath definitions
#process.reconstruction_step = cms.Path(process.reconstruction_fromRECO)
process.reconstruction_step = cms.Path(process.seedFromGlobalMuons + process.standAloneMuons)
process.endjob_step = cms.EndPath(process.endOfProcess)
process.RECOSIMoutput_step = cms.EndPath(process.RECOSIMoutput)

# Schedule definition
process.schedule = cms.Schedule(process.reconstruction_step,process.endjob_step,process.RECOSIMoutput_step)
from PhysicsTools.PatAlgos.tools.helpers import associatePatAlgosToolsTask
associatePatAlgosToolsTask(process)



# Customisation from command line

#Have logErrorHarvester wait for the same EDProducers to finish as those providing data for the OutputModule
from FWCore.Modules.logErrorHarvester_cff import customiseLogErrorHarvesterUsingOutputCommands
process = customiseLogErrorHarvesterUsingOutputCommands(process)

# Add early deletion of temporary data products to reduce peak memory need
from Configuration.StandardSequences.earlyDeleteSettings_cff import customiseEarlyDelete
process = customiseEarlyDelete(process)
# End adding early deletion
