import FWCore.ParameterSet.Config as cms

from RecoTracker.LST.lstsonicProducer_cfi import lstsonicProducer
from RecoTracker.LST.lstsonicOutputConverter_cfi import lstsonicOutputConverter

lstSONICProducer = lstsonicProducer.clone(
    Client = cms.PSet(
        timeout = cms.untracked.uint32(300),
        mode = cms.string("Async"),
        modelName = cms.string("LST"), # double check
        modelConfigPath = cms.FileInPath("RecoTracker/LST/data/config.pbtxt"), # this is SONIC, not currently in the CMSSW, so the models/ will be copied to this location privately
        modelVersion = cms.string(""),
        verbose = cms.untracked.bool(False),
        allowedTries = cms.untracked.uint32(0),
        useSharedMemory = cms.untracked.bool(True),
        compression = cms.untracked.string(""),
    ),
)

lstSONICProducerTask = cms.Task(lstSONICProducer, lstsonicOutputConverter) # YY: maybe it is not gonna work because the second is in alpaka while the first is not
