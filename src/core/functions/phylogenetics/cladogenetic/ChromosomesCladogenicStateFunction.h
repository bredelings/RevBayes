#ifndef ChromosomesCladogenicStateFunction_H
#define ChromosomesCladogenicStateFunction_H

#include "AbstractCladogenicStateFunction.h"
#include "CladogeneticProbabilityMatrix.h"
#include "RbVector.h"
#include "Simplex.h"
#include "TypedDagNode.h"
#include "TypedFunction.h"

#include <sstream>
#include <vector>
#include <map>

namespace RevBayesCore {

    class ChromosomesCladogenicStateFunction : public AbstractCladogenicStateFunction, public TypedFunction<CladogeneticProbabilityMatrix> {
        
    public:
        
        ChromosomesCladogenicStateFunction( const TypedDagNode< Simplex >* ep, unsigned mc );
        virtual                                            ~ChromosomesCladogenicStateFunction(void);               
        
        const static unsigned NO_CHANGE                             = 0;         // N -> N
        const static unsigned FISSION                               = 1;         // N -> N + 1
        const static unsigned FUSION                                = 2;         // N -> N - 1
        const static unsigned POLYPLOIDIZATION                      = 3;         // N -> N * 2
        const static unsigned DEMIPOLYPLOIDIZATION                  = 4;         // N -> N * 1.5
        
        // public member functions
        ChromosomesCladogenicStateFunction*                         clone(void) const;
        std::map< std::vector<unsigned>, double >                      getEventMap(double t=0.0);
        const std::map< std::vector<unsigned>, double >&                getEventMap(double t=0.0) const;
        
        void                                                        update(void);
        
    protected:
        
        void                                                        swapParameterInternal(const DagNode *oldP, const DagNode *newP);
        
    private:
        
        void                                                        buildEventMap(void);
        void                                                        updateProbs(void);
        
        // members
        const TypedDagNode< Simplex >*                              eventProbs;
        unsigned                                                    maxChromo;
        unsigned                                                    numEventTypes;
        std::map< std::vector<unsigned>, std::vector<unsigned> >    eventMapTypes;
        std::map< std::vector<unsigned>, double >                   eventMapProbs;
        std::vector< std::vector<unsigned> >                        eventMapCounts;

    };
    
}

#endif
