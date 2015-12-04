#ifndef PosteriorPredictiveAnalysis_H
#define PosteriorPredictiveAnalysis_H

#include "Cloneable.h"
#include "Parallelizable.h"
#include "RbVector.h"

namespace RevBayesCore {
    
    class MonteCarloAnalysis;
    
    /**
     * @brief Posterior predictive analysis class.
     *
     * A posterior predictive analysis runs an analysis for a vector of powers
     * where the likelihood during each analysis run is raised to the given power.
     * The likelihood values and the current powers are stored in a file.
     *
     *
     * @copyright Copyright 2009-
     * @author The RevBayes Development Core Team (Sebastian Hoehna)
     * @since Version 1.0, 2012-06-17
     *
     */
    class PosteriorPredictiveAnalysis : public Cloneable, public Parallelizable {
        
    public:
        PosteriorPredictiveAnalysis(const MonteCarloAnalysis &m, const std::string &dir);
        PosteriorPredictiveAnalysis(const PosteriorPredictiveAnalysis &a);
        virtual                                ~PosteriorPredictiveAnalysis(void);                               //!< Virtual destructor
        
        PosteriorPredictiveAnalysis&            operator=(const PosteriorPredictiveAnalysis &a);
        
        // public methods
        PosteriorPredictiveAnalysis*            clone(void) const;
        void                                    burnin(size_t g, size_t ti);
        void                                    runAll(size_t g);
        void                                    runSim(size_t idx, size_t g);
        
    private:
        
        // members
        std::string                             directory;
        size_t                                  num_runs;
        std::vector<MonteCarloAnalysis*>        runs;
        
    };
    
}

#endif
