#ifndef HeterogeneousRateBirthDeath_H
#define HeterogeneousRateBirthDeath_H

#include "AbstractCharacterHistoryBirthDeathProcess.h"
#include "CharacterHistory.h"
#include "MemberObject.h"
#include "Tree.h"
#include "TypedDagNode.h"
#include "TypedDistribution.h"

namespace RevBayesCore {
    
    class Clade;
    
    class HeterogeneousRateBirthDeath : public AbstractCharacterHistoryBirthDeathProcess, public MemberObject< RbVector<int> >, public MemberObject< RbVector<double> > {
        
    public:
        HeterogeneousRateBirthDeath(const TypedDagNode<double> *a, const TypedDagNode<int> *rs, const TypedDagNode<RbVector<double> > *s, const TypedDagNode<RbVector<double> > *e, const TypedDagNode<double> *ev, const TypedDagNode<double> *r, const std::vector<Taxon> &n);                                                                                  //!< Constructor
        
        virtual                                            ~HeterogeneousRateBirthDeath(void);                          //!< Virtual destructor
        
        // public member functions
        HeterogeneousRateBirthDeath*                        clone(void) const;                                          //!< Create an independent clone
        double                                              computeLnProbability(void);                                 //!< Compute ln prob of current value
        void                                                executeMethod(const std::string &n, const std::vector<const DagNode*> &args, RbVector<int> &rv) const;     //!< Map the member methods to internal function calls
        void                                                executeMethod(const std::string &n, const std::vector<const DagNode*> &args, RbVector<double> &rv) const;     //!< Map the member methods to internal function calls
        CharacterHistory&                                   getCharacterHistory(void);                                  //!< Get the character histories
        void                                                redrawValue(void);                                          //!< Draw a new random value from distribution
        void                                                setValue(Tree *v, bool force);
        
    protected:
        // Parameter management functions
        virtual void                                        getAffected(RbOrderedSet<DagNode *>& affected, DagNode* affecter);                                      //!< get affected nodes
        virtual void                                        keepSpecialization(DagNode* affecter);
        virtual void                                        restoreSpecialization(DagNode *restorer);
        virtual void                                        touchSpecialization(DagNode *toucher, bool touchAll);
        
        void                                                computeNodeProbability(const TopologyNode &n, size_t nIdx);
        double                                              computeRootLikelihood(void);
        
        
        void                                                swapParameterInternal(const DagNode *oldP, const DagNode *newP);            //!< Swap a parameter
        
    private:
        
        // helper functions
        void                                                attachTimes(Tree *psi, std::vector<TopologyNode *> &tips, size_t index, const std::vector<double> &times, double T);
        void                                                buildRandomBinaryHistory(std::vector<TopologyNode *> &tips);
        size_t                                              computeStartIndex(size_t i) const;
        void                                                simulateTree(void);
        void                                                runMCMC(void);
        void                                                updateBranchProbabilitiesNumerically(std::vector<double> &state, double begin, double end, const RbVector<double> &s, const RbVector<double> &e, double r, size_t i);

        // members
        const TypedDagNode<double>*                         root_age;
        const TypedDagNode<int>*                            root_state;
        const TypedDagNode< RbVector<double> >*             speciation;
        const TypedDagNode< RbVector<double> >*             extinction;
        const TypedDagNode<double>*                         event_rate;
        const TypedDagNode<double>*                         rho;                                                                                                //!< Sampling probability of each species.

        CharacterHistory                                    branch_histories;
        
        size_t                                              num_taxa;
        size_t                                              num_rate_categories;
        std::vector<Taxon>                                  taxa;
    
    
        std::vector<size_t>                                 activeLikelihood;
        mutable std::vector<bool>                           changedNodes;
        mutable std::vector<bool>                           dirtyNodes;
        mutable std::vector<std::vector<std::vector<double> > >       nodeStates;
        mutable std::vector<std::vector<double> >           scalingFactors;
        mutable double                                      totalScaling;

        double                                              logTreeTopologyProb;                                                                                //!< Log-transformed tree topology probability (combinatorial constant).

        const double                                        NUM_TIME_SLICES;

    };
    
}

#endif
