#ifndef SimG4Core_PhysicsLists_FTFPCMS_BERT_ATL_EMM_H
#define SimG4Core_PhysicsLists_FTFPCMS_BERT_ATL_EMM_H

#include "SimG4Core/Physics/interface/PhysicsList.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"

class FTFPCMS_BERT_ATL_EMM : public PhysicsList {
public:
  FTFPCMS_BERT_ATL_EMM(const edm::ParameterSet& p);
};

#endif
