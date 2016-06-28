#ifndef CharacterDependentCladoBirthDeathProcess_H
#define CharacterDependentCladoBirthDeathProcess_H

#include "TreeDiscreteCharacterData.h"
#include "CDCladoSE.h"
#include "MatrixReal.h"
#include "RateMatrix.h"
#include "Taxon.h"
#include "Tree.h"
#include "TypedDagNode.h"


#include <vector>

namespace RevBayesCore {
    
    class Clade;
    
    /**
     * @file
     * This file contains the declaration of the random variable class for the character-dependent
     * cladogenetic birth-death process: ClaSSE as described in Golberg & Igic 2012
     *
     * Will Freyman 6/22/16
     *
     */
    class CharacterDependentCladoBirthDeathProcess : public TypedDistribution<Tree> {
        
    public:
        CharacterDependentCladoBirthDeathProcess(const TypedDagNode<double> *ro,
                                            const TypedDagNode<RbVector<double> >* mo,
                                            const TypedDagNode<RateGenerator>* q, const TypedDagNode<double>* r, const TypedDagNode< RbVector< double > >* p,
                                            const TypedDagNode<double> *rh, const std::string &cdt, const std::vector<Taxon> &tn);
        
        // pure virtual member functions
        virtual CharacterDependentCladoBirthDeathProcess*        clone(void) const;                                                                                  //!< Create an independent clone
        
        // public member functions you may want to override
        double                                              computeLnProbability(void);                                                                         //!< Compute the log-transformed probability of the current value.
        virtual void                                        redrawValue(void);                                                                                  //!< Draw a new random value from the distribution
        void                                                setCladogenesisMatrix(const TypedDagNode< RbVector< MatrixReal> >* r);
        void                                                setCladogenesisMatrix(const TypedDagNode< MatrixReal > *r);
        virtual void                                        setValue(Tree *v, bool f=false);                                                                    //!< Set the current value, e.g. attach an observation (clamp)
        
        
    protected:
        
        // virtual methods that may be overwritten, but then the derived class should call this methods
        virtual void                                        getAffected(RbOrderedSet<DagNode *>& affected, DagNode* affecter);                                  //!< get affected nodes
        virtual void                                        keepSpecialization(DagNode* affecter);
        virtual void                                        restoreSpecialization(DagNode *restorer);
        virtual void                                        touchSpecialization(DagNode *toucher, bool touchAll);
        
        // Parameter management functions. You need to override both if you have additional parameters
        virtual void                                        swapParameterInternal(const DagNode *oldP, const DagNode *newP);                                    //!< Swap a parameter
        void                                                executeProcedure(const std::string &name, const std::vector<DagNode *> args, bool &found);
        
        // helper functions
        void                                                buildRandomBinaryTree(std::vector<TopologyNode *> &tips);
        void                                                prepareProbComputation(void) const;
        void                                                simulateTree(void);
        void                                                computeNodeProbability(const TopologyNode &n, size_t nIdx) const;
        double                                              computeRootLikelihood() const;
        
        // members
        bool                                                branchHeterogeneousCladogenesis;
        std::string                                         condition;                                                                                          //!< The condition of the process (none/survival/#taxa).
        size_t                                              num_taxa;                                                                                            //!< Number of taxa (needed for correct initialization).
        std::vector<Taxon>                                  taxa;                                                                                               //!< Taxon names that will be attached to new simulated trees.
        double                                              log_tree_topology_prob;                                                                                //!< Log-transformed tree topology probability (combinatorial constant).
        std::vector<size_t>                                 active_likelihood;
        mutable std::vector<bool>                           changed_nodes;
        mutable std::vector<bool>                           dirty_nodes;
        mutable std::vector<std::vector<state_type> >       node_states;
        size_t                                              num_rate_categories;
        size_t                                              num_observed_states;
        
        // parameters
        const TypedDagNode< MatrixReal >*                   homogeneousCladogenesisMatrix;
        const TypedDagNode< RbVector< MatrixReal > >*       heterogeneousCladogenesisMatrices;
        
        const TypedDagNode<double>*                         root_age;                                                                                            //!< Time since the origin.
        const TypedDagNode<RbVector<double> >*              mu;
        const TypedDagNode< RbVector< double > >*           pi;                                                                                                 //!< The root frequencies (probabilities of the root states).
        const TypedDagNode<RateGenerator>*                  Q;
        const TypedDagNode<double>*                         rate;                                                                                                //!< Sampling probability of each species.
        const TypedDagNode<double>*                         rho;                                                                                                //!< Sampling probability of each species.
        
        mutable std::vector<double>                         extinction_rates;
        
        const double                                        NUM_TIME_SLICES;
    };
    
}

#endif