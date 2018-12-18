//
//  BiogeographyCladogeneticBirthDeathFunction.cpp
//  revbayes-proj
//
//  Created by Michael Landis on 12/15/18.
//  Copyright © 2018 Michael Landis. All rights reserved.
//

#include "BiogeographyCladogeneticBirthDeathFunction.h"
#include "CladogeneticSpeciationRateMatrix.h"
#include "MatrixReal.h"
#include "RbException.h"
#include <cmath>


using namespace RevBayesCore;


//TypedFunction<MatrixReal>( new MatrixReal( mc + 1, (mc + 1) * (mc + 1), 0.0 ) ),
BiogeographyCladogeneticBirthDeathFunction::BiogeographyCladogeneticBirthDeathFunction( const TypedDagNode< RbVector< double > >* sr,
                                                                                        unsigned mrs,
                                                                                        TypedDagNode< RbVector< RbVector<double> > >* cm,
                                                                                        TypedDagNode< RbVector< double > >* cw,
                                                                                        std::string ct ):
TypedFunction<CladogeneticSpeciationRateMatrix>( new CladogeneticSpeciationRateMatrix( mrs ) ),
speciationRates( sr ),
connectivityMatrix( cm ),
connectivityWeights( cw ),
numCharacters( (unsigned)cm->getValue().size() ),
num_states( 2 ),
numIntStates( pow(2,cm->getValue().size()) ),
maxRangeSize(mrs),
numEventTypes( (unsigned)sr->getValue().size() ),
use_hidden_rate(false),
connectivityType( ct )
{
    addParameter( speciationRates );
    addParameter( connectivityMatrix );
    addParameter( connectivityWeights );
    
    
    if (numCharacters > 10)
    {
        throw RbException(">10 characters currently unsupported");
    }
    
    buildBits();
    buildRanges(ranges, connectivityMatrix, true);
    
    numRanges = (unsigned)ranges.size();
    numRanges++; // add one for the null range
    
    buildEventMap();
    update();
    
}


BiogeographyCladogeneticBirthDeathFunction::~BiogeographyCladogeneticBirthDeathFunction( void ) {
    // We don't delete the parameters, because they might be used somewhere else too. The model needs to do that!
}

std::vector<unsigned> BiogeographyCladogeneticBirthDeathFunction::bitAllopatryComplement( const std::vector<unsigned>& mask, const std::vector<unsigned>& base )
{
    std::vector<unsigned> ret = mask;
    for (size_t i = 0; i < base.size(); i++)
    {
        if (base[i] == 1)
            ret[i] = 0;
    }
    return ret;
}

void BiogeographyCladogeneticBirthDeathFunction::bitCombinations(std::vector<std::vector<unsigned> >& comb, std::vector<unsigned> array, int i, std::vector<unsigned> accum)
{
    if (i == array.size()) // end recursion
    {
        unsigned n = sumBits(accum);
        
        if ( n == 0 || n == sumBits(array) )
            ;  // ignore all-0, all-1 vectors
        else
            comb.push_back(accum);
    }
    else
    {
        unsigned b = array[i];
        std::vector<unsigned> tmp0(accum);
        std::vector<unsigned> tmp1(accum);
        tmp0.push_back(0);
        bitCombinations(comb,array,i+1,tmp0);
        if (b == 1)
        {
            tmp1.push_back(1);
            bitCombinations(comb,array,i+1,tmp1);
        }
    }
}

unsigned BiogeographyCladogeneticBirthDeathFunction::bitsToState( const std::vector<unsigned>& b )
{
    return bitsToStatesByNumOn[b];
}

std::string BiogeographyCladogeneticBirthDeathFunction::bitsToString( const std::vector<unsigned>& b )
{
    std::stringstream ss;
    for (size_t i = 0; i < b.size(); i++)
    {
        ss << b[i];
    }
    return ss.str();
}

void BiogeographyCladogeneticBirthDeathFunction::buildBits( void )
{
    
    eventTypes.push_back("s");
    eventTypes.push_back("a");
    for (size_t i = 0; i < eventTypes.size(); i++) {
        if (eventTypes[i]=="s")
            eventStringToStateMap[ eventTypes[i] ] = SYMPATRY;
        else if (eventTypes[i]=="a")
            eventStringToStateMap[ eventTypes[i] ] = ALLOPATRY;
        else if (eventTypes[i]=="j")
            eventStringToStateMap[ eventTypes[i] ] = JUMP_DISPERSAL;
    }
    
    bitsByNumOn.resize(numCharacters+1);
    statesToBitsByNumOn.resize(numIntStates);
    statesToBitsetsByNumOn.resize(numIntStates);
    bits = std::vector<std::vector<unsigned> >(numIntStates, std::vector<unsigned>(numCharacters, 0));
    bitsByNumOn[0].push_back(bits[0]);
    for (size_t i = 1; i < numIntStates; i++)
    {
        size_t m = i;
        for (size_t j = 0; j < numCharacters; j++)
        {
            bits[i][j] = m % 2;
            m /= 2;
            if (m == 0)
                break;
        }
        size_t j = sumBits(bits[i]);
        bitsByNumOn[j].push_back(bits[i]);
        
    }
    for (size_t i = 0; i < numIntStates; i++)
    {
        inverseBits[ bits[i] ] = (unsigned)i;
    }
    
    // assign state to each bit vector, sorted by numOn
    size_t k = 0;
    for (size_t i = 0; i < bitsByNumOn.size(); i++)
    {
        for (size_t j = 0; j < bitsByNumOn[i].size(); j++)
        {
            // assign to presence-absence vector
            statesToBitsByNumOn[k] = bitsByNumOn[i][j];
            
            // assign to set of present areas
            std::set<unsigned> s;
            for (size_t m = 0; m < statesToBitsByNumOn[k].size(); m++)
            {
                if (statesToBitsByNumOn[k][m] == 1) {
                    s.insert( m );
                }
            }
            statesToBitsetsByNumOn[k] = s;
            k++;
        }
    }
    
    for (size_t i = 0; i < statesToBitsByNumOn.size(); i++)
    {
        bitsToStatesByNumOn[ statesToBitsByNumOn[i] ] = (unsigned)i;
    }
    
}


void BiogeographyCladogeneticBirthDeathFunction::buildCutsets( void ) {
    
    std::map< std::vector<unsigned>, unsigned>::iterator it;
    
    for (it = eventMapTypes.begin(); it != eventMapTypes.end(); it++)
    {
        // get event
        std::vector<unsigned> idx = it->first;
        unsigned event_type = it->second;
        
        // get right and left bitsets
        std::set<unsigned> r1 = statesToBitsetsByNumOn[ idx[1] ];
        std::set<unsigned> r2 = statesToBitsetsByNumOn[ idx[2] ];
     
        // fill vector with edges to cut
        std::vector< std::vector<unsigned> > cutset;
        
//        if (event_type == SYMPATRY) {
//            // find the sympatric area
//            const std::set<unsigned>& s1 = ( r1.size() < r2.size() ? r1 : r2 );
//            const std::set<unsigned>& s2 = ( r1.size() >= r2.size() ? r1 : r2 );
//            std::set<unsigned>::iterator jt, kt;
//            for (jt = s1.begin(); jt != s1.end(); jt++)
//            {
//                for (kt = s2.begin(); kt != s2.end(); kt++)
//                {
//                    std::vector<unsigned> edge;
//                    edge.push_back( *jt );
//                    edge.push_back( *kt );
//                    cutset.push_back( edge );
//                }
//            }
//        }
//        else if (event_type == ALLOPATRY) {
            // find the sympatric area
            const std::set<unsigned>& s1 = r1; //( r1.size() < r2.size() ? r1 : r2 );
            const std::set<unsigned>& s2 = r2; // ( r1.size() >= r2.size() ? r1 : r2 );
            std::set<unsigned>::iterator jt, kt;
            for (jt = s1.begin(); jt != s1.end(); jt++)
            {
                for (kt = s2.begin(); kt != s2.end(); kt++)
                {
                    if ( *jt != *kt )
                    {
                        std::vector<unsigned> edge;
                        edge.push_back( *jt );
                        edge.push_back( *kt );
                        cutset.push_back( edge );
                    }
                }
            }
//        }
        eventMapCutsets[ idx ] = cutset;
    }
    
    std::cout << "";
}

/*
 *  This function populates the eventMap, eventMapTypes, and eventMapCounts
 *  so it may be rapidly filled with values when update() is called.
 */

void BiogeographyCladogeneticBirthDeathFunction::buildEventMap( void ) {
    
    // clear events
    eventMapCounts.clear();
    
    // get L,R states per A state
    std::vector<unsigned> idx(3);

    // loop over possible ranges
    for (std::set<unsigned>::iterator its = ranges.begin(); its != ranges.end(); its++)
    {
        unsigned i = *its;
        idx[0] = i;
        eventMapCounts[i] = std::vector<unsigned>(NUM_CLADO_EVENT_TYPES, 0);
        
#ifdef DEBUG_DEC
        std::cout << "State " << i << "\n";
        std::cout << "Bits  " << bitsToString(statesToBitsByNumOn[i]) << "\n";
#endif
        
        // get on bits for A
        const std::vector<unsigned>& ba = statesToBitsByNumOn[i];
        std::vector<unsigned> on;
        std::vector<unsigned> off;
        for (unsigned j = 0; j < ba.size(); j++)
        {
            if (ba[j] == 1)
                on.push_back(j);
            else
                off.push_back(j);
        }
        
        std::vector<unsigned> bl(numCharacters, 0);
        std::vector<unsigned> br(numCharacters, 0);
        
        // narrow sympatry
        if (sumBits(ba) == 1)
        {
            
#ifdef DEBUG_DEC
            
#endif
            idx[1] = i;
            idx[2] = i;
            if (ranges.find(i) == ranges.end())
            {
                continue;
            }
            
            eventMapTypes[ idx ] = SYMPATRY;
            eventMapCounts[ i ][  SYMPATRY ] += 1;
            eventMap[ idx ] = 0.0;
            
#ifdef DEBUG_DEC
            std::cout << "Narrow sympatry\n";
            std::cout << "A " << bitsToState(statesToBitsByNumOn[i]) << " " << bitsToString(statesToBitsByNumOn[i]) << "\n";
            std::cout << "L " << bitsToState(statesToBitsByNumOn[i]) << " " << bitsToString(statesToBitsByNumOn[i]) << "\n";
            std::cout << "R " << bitsToState(statesToBitsByNumOn[i]) << " " << bitsToString(statesToBitsByNumOn[i]) << "\n\n";
#endif
            
        }
        
        // subset/widespread sympatry
        else if (sumBits(ba) > 1)
        {
            idx[1] = i;
            idx[2] = i;
            if (ranges.find(i) == ranges.end())
            {
                continue;
            }
            
            if (eventStringToStateMap.find("s") != eventStringToStateMap.end()) {
                
#ifdef DEBUG_DEC
                std::cout << "Subset sympatry (L-trunk, R-bud)\n";
#endif
                
                // get set of possible sympatric events for L-trunk, R-bud
                for (size_t j = 0; j < on.size(); j++)
                {
                    br = std::vector<unsigned>(numCharacters, 0);
                    br[ on[j] ] = 1;
                    //                unsigned sr = bitsToState(br);
                    unsigned sr = bitsToStatesByNumOn[br];
                    idx[1] = i;
                    idx[2] = sr;
                    
                    if (ranges.find(sr) == ranges.end())
                    {
                        br[ on[j] ] = 0;
                        continue;
                    }
                    
                    eventMapTypes[ idx ] = SYMPATRY;
                    eventMapCounts[ i ][  SYMPATRY ] += 1;
                    eventMap[ idx ] = 0.0;
                    
#ifdef DEBUG_DEC
                    std::cout << "A " << bitsToState(statesToBitsByNumOn[i]) << " " << bitsToString(statesToBitsByNumOn[i]) << "\n";
                    std::cout << "L " << bitsToState(statesToBitsByNumOn[i]) << " " << bitsToString(statesToBitsByNumOn[i]) << "\n";
                    std::cout << "R " << bitsToState(br) << " " << bitsToString(br) << "\n\n";
#endif
                    
                    br[ on[j] ] = 0;
                }
                
                
#ifdef DEBUG_DEC
                std::cout << "Subset sympatry (L-bud, R-trunk)\n";
#endif
                
                // get set of possible sympatric events for R-trunk, L-bud
                for (size_t j = 0; j < on.size(); j++)
                {
                    bl = std::vector<unsigned>(numCharacters, 0);
                    
                    bl[ on[j] ] = 1;
                    //                unsigned sl = bitsToState(bl);
                    unsigned sl = bitsToStatesByNumOn[bl];
                    idx[1] = sl;
                    idx[2] = i;
                    
                    if (ranges.find(sl) == ranges.end())
                    {
                        bl[ on[j] ] = 0;
                        continue;
                    }
                    
                    eventMapTypes[ idx ] =  SYMPATRY;
                    eventMapCounts[ i ][  SYMPATRY ] += 1;
                    eventMap[ idx ] = 0.0;
                    
#ifdef DEBUG_DEC
                    std::cout << "A " << bitsToState(statesToBitsByNumOn[i]) << " "<< bitsToString(statesToBitsByNumOn[i]) << "\n";
                    std::cout << "L " << bitsToState(bl) << " "<< bitsToString(bl) << "\n";
                    std::cout << "R " << bitsToState(statesToBitsByNumOn[i]) << " " << bitsToString(statesToBitsByNumOn[i]) << "\n\n";
#endif
                    
                    bl[ on[j] ] = 0;
                }
            }
            
            
            if (eventStringToStateMap.find("a") != eventStringToStateMap.end()) {
                
                // get set of possible allopatry events
                bl = ba;
                std::vector<std::vector<unsigned> > bc;
                bitCombinations(bc, ba, 0, std::vector<unsigned>());
                
#ifdef DEBUG_DEC
                std::cout << "Allopatry combinations\n";
                std::cout << "A " << bitsToState(ba) << " " << bitsToString(ba) << "\n";
#endif
                
                for (size_t j = 0; j < bc.size(); j++)
                {
                    
                    bl = bc[j];
                    br = bitAllopatryComplement(ba, bl);
                    
                    if (sumBits(bl)==1 || sumBits(br)==1)
                    {
                        
                        unsigned sa = bitsToStatesByNumOn[ba];
                        unsigned sl = bitsToStatesByNumOn[bl];
                        unsigned sr = bitsToStatesByNumOn[br];
                        idx[1] = sl;
                        idx[2] = sr;
                        
#ifdef DEBUG_DEC
                        std::cout << "L " << bitsToState(bl) << " " << bitsToString(bl) << "\n";
                        std::cout << "R " << bitsToState(br) << " " << bitsToString(br) << "\n";
#endif
                        
                        
                        eventMapTypes[ idx ] = ALLOPATRY;
                        eventMapCounts[ i ][  ALLOPATRY ] += 1;
                        eventMap[ idx ] = 0.0;
                        
#ifdef DEBUG_DEC
                        std::cout << "\n";
#endif
                    }
                }
            }
        }
        
        // jump dispersal
        if (eventStringToStateMap.find("j") != eventStringToStateMap.end()) {
            
#ifdef DEBUG_DEC
            std::cout << "Jump dispersal (L-trunk, R-bud)\n";
#endif
            
            // get set of possible jump dispersal events for L-trunk, R-bud
            for (size_t j = 0; j < off.size(); j++)
            {
                br = std::vector<unsigned>(numCharacters, 0);
                br[ off[j] ] = 1;
                //                unsigned sr = bitsToState(br);
                unsigned sr = bitsToStatesByNumOn[br];
                idx[1] = i;
                idx[2] = sr;
                
                if (ranges.find(sr) == ranges.end())
                {
                    br[ off[j] ] = 0;
                    continue;
                }
                
                
                eventMapTypes[ idx ] = JUMP_DISPERSAL;
                eventMapCounts[ i ][  JUMP_DISPERSAL ] += 1;
                eventMap[ idx ] = 0.0;
                
#ifdef DEBUG_DEC
                std::cout << "A " << bitsToState(statesToBitsByNumOn[i]) << " " << bitsToString(statesToBitsByNumOn[i]) << "\n";
                std::cout << "L " << bitsToState(statesToBitsByNumOn[i]) << " " << bitsToString(statesToBitsByNumOn[i]) << "\n";
                std::cout << "R " << bitsToState(br) << " " << bitsToString(br) << "\n\n";
#endif
                
                br[ off[j] ] = 0;
            }
            
            
#ifdef DEBUG_DEC
            std::cout << "Jump dispersal (L-bud, R-trunk)\n";
#endif
            
            // get set of possible jump dispersal events for R-trunk, L-bud
            for (size_t j = 0; j < off.size(); j++)
            {
                bl = std::vector<unsigned>(numCharacters, 0);
                
                bl[ off[j] ] = 1;
                //                unsigned sl = bitsToState(bl);
                unsigned sl = bitsToStatesByNumOn[bl];
                idx[1] = sl;
                idx[2] = i;
                
                if (ranges.find(sl) == ranges.end())
                {
                    bl[ off[j] ] = 0;
                    continue;
                }
                
                eventMapTypes[ idx ] =  JUMP_DISPERSAL;
                eventMapCounts[ i ][  JUMP_DISPERSAL ] += 1;
                eventMap[ idx ] = 0.0;
                
#ifdef DEBUG_DEC
                std::cout << "A " << bitsToState(statesToBitsByNumOn[i]) << " "<< bitsToString(statesToBitsByNumOn[i]) << "\n";
                std::cout << "L " << bitsToState(bl) << " "<< bitsToString(bl) << "\n";
                std::cout << "R " << bitsToState(statesToBitsByNumOn[i]) << " " << bitsToString(statesToBitsByNumOn[i]) << "\n\n";
#endif
                
                bl[ off[j] ] = 0;
            }
        }
#ifdef DEBUG_DEC
        std::cout << "\n\n";
#endif
    }
#ifdef DEBUG_DEC
    //    for (size_t i = 0; i < eventMapCounts.size(); i++) {
    //        std::cout << bitsToState(statesToBitsByNumOn[i]) << " " << eventMapCounts[ i ] << "\n";
    //    }
    //
    std::cout << "------\n";
#endif
    
    buildCutsets();
}

void BiogeographyCladogeneticBirthDeathFunction::buildRanges(std::set<unsigned>& range_set, const TypedDagNode< RbVector<RbVector<double> > >* g, bool all)
{
    
    std::set<std::set<unsigned> > r;
    for (size_t i = 0; i < numCharacters; i++)
    {
        std::set<unsigned> s;
        s.insert((unsigned)i);
        r.insert(s);
        buildRangesRecursively(s, r, maxRangeSize, g, all);
    }
    
    for (std::set<std::set<unsigned> >::iterator it = r.begin(); it != r.end(); it++)
    {
        std::vector<unsigned> v(numCharacters, 0);
        for (std::set<unsigned>::iterator jt = it->begin(); jt != it->end(); jt++)
        {
            v[*jt] = 1;
        }
        range_set.insert( bitsToStatesByNumOn[v] );
    }
    
#ifdef DEBUG_DEC
    for (std::set<std::set<unsigned> >::iterator it = r.begin(); it != r.end(); it++)
    {
        for (std::set<unsigned>::iterator jt = it->begin(); jt != it->end(); jt++)
        {
            std::cout << *jt << " ";
        }
        std::cout << "\n";
    }
    std::cout << "\n";
#endif
}

void BiogeographyCladogeneticBirthDeathFunction::buildRangesRecursively(std::set<unsigned> s, std::set<std::set<unsigned> >& r, size_t k, const TypedDagNode< RbVector<RbVector<double> > >* g, bool all)
{
    
    // add candidate range to list of ranges
    if (s.size() <= k)
        r.insert(s);
    
    // stop recursing if range equals max size, k
    if (s.size() == k)
        return;
    
    
    // otherwise, recurse along
    for (std::set<unsigned>::iterator it = s.begin(); it != s.end(); it++)
    {
        for (size_t i = 0; i < numCharacters; i++)
        {
            if (g->getValue()[*it][i] > 0 || all) {
                std::set<unsigned> t = s;
                t.insert((unsigned)i);
                if (r.find(t) == r.end())
                {
                    buildRangesRecursively(t, r, k, g, all);
                }
            }
        }
    }
}



BiogeographyCladogeneticBirthDeathFunction* BiogeographyCladogeneticBirthDeathFunction::clone( void ) const
{
    return new BiogeographyCladogeneticBirthDeathFunction( *this );
}


double BiogeographyCladogeneticBirthDeathFunction::computeDataAugmentedCladogeneticLnProbability(const std::vector<BranchHistory*>& histories,
                                                                                              size_t node_index,
                                                                                              size_t left_index,
                                                                                              size_t right_index ) const
{
    throw RbException("BiogeographyCladogeneticBirthDeathFunction::computeDataAugmentedCladogeneticLnProbability is not currently implemented.");
    double lnP = 0.0;
    return lnP;
    
}


double BiogeographyCladogeneticBirthDeathFunction::computeModularityScore(unsigned state1, unsigned state2, unsigned event_type)
{
    // return value
    double Q = 0.0;
    
    // get value for connectivity mtx
    const RbVector<RbVector<double> >& mtx = connectivityMatrix->getValue();
    size_t n = mtx.size();
    
    // get left/right area sets
    std::vector< std::set<unsigned> > daughter_ranges;
    daughter_ranges.push_back( statesToBitsetsByNumOn[ state1 ] );
    daughter_ranges.push_back( statesToBitsetsByNumOn[ state2 ] );
    
    // compute modularity score depending on event type
    if (event_type == SYMPATRY)
    {
        
        if (daughter_ranges[0].size() == 1 && daughter_ranges[1].size() == 1)
        {
            return 0.0;
        }
        
        std::vector<double> z(n, 0.0);
        
        // get trunk range (larger range)
        const std::set<unsigned>& s = ( daughter_ranges[0].size() > daughter_ranges[1].size() ? daughter_ranges[0] : daughter_ranges[1] );
        
        // compute z, the sum of ranges for the trunk range
        std::set<unsigned>::iterator it1, it2;
        for (it1 = s.begin(); it1 != s.end(); it1++)
        {
            for (it2 = s.begin(); it2 != s.end(); it2++)
            {
                if ( (*it1) != (*it2) ) {
                    z[ *it1 ] += mtx[ *it1 ][ *it2 ];
                }
            }
        }
        
        double z_sum = 0.0;
        for (size_t i = 0; i < z.size(); i++) {
            z_sum += z[i];
        }
        
        for (size_t i = 0; i < n; i++)
        {
            for (size_t j = 0; j < n; j++)
            {
                if (i != j) {
                    Q += mtx[i][j] - (z[i] * z[j] / (2 * z_sum));
                }
            }
        }
        
    }
    else if (event_type == ALLOPATRY)
    {
        std::vector<double> z(n, 0.0);
        
        // compute z, the sum of edges across daughter ranges
        for (size_t i = 0; i < daughter_ranges.size(); i++) {
        
            // get one daughter range
            const std::set<unsigned>& s = daughter_ranges[i];
            
            // sum connectivity scores
            std::set<unsigned>::iterator it1, it2;
            for (it1 = s.begin(); it1 != s.end(); it1++)
            {
                for (it2 = s.begin(); it2 != s.end(); it2++)
                {
                    if ( (*it1) != (*it2) ) {
                        z[ *it1 ] += mtx[ *it1 ][ *it2 ];
                    }
                }
            }
        }
        
        double z_sum = 0.0;
        for (size_t i = 0; i < z.size(); i++) {
            z_sum += z[i];
        }
        
        for (size_t i = 0; i < n; i++)
        {
            for (size_t j = 0; j < n; j++)
            {
                if (i != j) {
                    Q += mtx[i][j] - (z[i] * z[j] / (2 * z_sum));
                }
            }
        }
        
    }
    
    return Q;
}




std::map< std::vector<unsigned>, double >  BiogeographyCladogeneticBirthDeathFunction::getEventMap(double t)
{
    return eventMap;
}

const std::map< std::vector<unsigned>, double >&  BiogeographyCladogeneticBirthDeathFunction::getEventMap(double t) const
{
    return eventMap;
}


void BiogeographyCladogeneticBirthDeathFunction::printEventMap(void)
{
    std::map< std::vector< unsigned >, double >::iterator it;
    for (it = eventMap.begin(); it != eventMap.end(); it++)
    {
        std::vector<unsigned> idx = it->first;
        double rate = it->second;
        unsigned event_type = eventMapTypes[ idx ];
        std::cout << idx[0] << " -> " << idx[1] << " | " << idx[2] << " : " << event_type << " = " << rate << "\n";
    }
}



void BiogeographyCladogeneticBirthDeathFunction::setRateMultipliers(const TypedDagNode< RbVector< double > >* rm)
{
    
    if (rm != NULL) {
        hiddenRateMultipliers = rm;
        addParameter( hiddenRateMultipliers );
        use_hidden_rate = true;
        
        buildEventMap();
        update();
    }
}


unsigned BiogeographyCladogeneticBirthDeathFunction::sumBits(const std::vector<unsigned>& b)
{
    unsigned n = 0;
    for (int i = 0; i < b.size(); i++)
        n += b[i];
    return n;
}



void BiogeographyCladogeneticBirthDeathFunction::swapParameterInternal(const DagNode *oldP, const DagNode *newP)
{
    if (oldP == speciationRates)
    {
        speciationRates = static_cast<const TypedDagNode< RbVector<double> >* >( newP );
    }
    if (oldP == hiddenRateMultipliers)
    {
        hiddenRateMultipliers = static_cast<const TypedDagNode< RbVector<double> >* >( newP );
    }
    if (oldP == connectivityMatrix)
    {
        connectivityMatrix = static_cast<const TypedDagNode< RbVector<RbVector<double> > >* >( newP );
    }
    if (oldP == connectivityWeights)
    {
        connectivityWeights = static_cast<const TypedDagNode< RbVector<double> >* >( newP );
    }
}



void BiogeographyCladogeneticBirthDeathFunction::update( void )
{
    // reset the transition matrix
    delete value;
    
    // check for a hidden rate category
    if (use_hidden_rate) {
        value = new CladogeneticSpeciationRateMatrix( numRanges * 2 );
    } else {
        value = new CladogeneticSpeciationRateMatrix( numRanges );
    }
    
    // update clado event factors
    updateEventMapFactors();
    
    // get speciation rates across cladogenetic events
    const std::vector<double>& sr = speciationRates->getValue();
    
    // assign the correct rate to each event
    for (unsigned i = 0; i < numRanges; i++)
    {
        std::map<std::vector<unsigned>, unsigned>::iterator it;
        for (it = eventMapTypes.begin(); it != eventMapTypes.end(); it++)
        {
            const std::vector<unsigned>& idx = it->first;
            eventMap[ idx ] = 0.0;
            if (use_hidden_rate == true)
            {
                std::vector<unsigned> idx_hidden(3);
                idx_hidden[0] = idx[0] + numRanges + 1;
                idx_hidden[1] = idx[1] + numRanges + 1;
                idx_hidden[2] = idx[2] + numRanges + 1;
                eventMap[ idx_hidden ] = 0.0;
            }
        }
        
        for (it = eventMapTypes.begin(); it != eventMapTypes.end(); it++)
        {
            const std::vector<unsigned>& idx = it->first;
            unsigned event_type = it->second;
            double speciation_rate = 0.0;
            
            // check for NaN values
            if (sr[ event_type ] == sr[ event_type ])
            {
                speciation_rate = sr[ event_type ];
            }
            
            // normalize for all possible instances of this event type
            double v = ( speciation_rate / eventMapCounts[ idx[0] ][ event_type ] );
            
            // save the rate in the event map
            eventMap[ idx ] += v;
            if (use_hidden_rate == true)
            {
                std::vector<unsigned> idx_hidden(3);
                idx_hidden[0] = idx[0] + numRanges + 1;
                idx_hidden[1] = idx[1] + numRanges + 1;
                idx_hidden[2] = idx[2] + numRanges + 1;
                const std::vector<double>& rate_multipliers = hiddenRateMultipliers->getValue();
                eventMap[ idx_hidden ] += (v * rate_multipliers[0]);
            }
        }
    }
    
    //    printEventMap();
    
    // done!
    value->setEventMap(eventMap);
}

void BiogeographyCladogeneticBirthDeathFunction::updateEventMapFactors(void)
{
    if ( connectivityType == "none" )
    {
        ; // do nothing
    }
    else if ( connectivityType == "modularity" )
    {
        updateEventMapModularityFactors();
    }
    else
    {
        throw RbException("Unknown connectivityType");
    }
    
}

void BiogeographyCladogeneticBirthDeathFunction::updateEventMapModularityFactors(void) {
    
    // loop over all events and their types
    std::map< std::vector<unsigned>, unsigned >::iterator it;
    for (it = eventMapTypes.begin(); it != eventMapTypes.end(); it++)
    {
        // get event info
        std::vector<unsigned> idx = it->first;
        unsigned event_type = it->second;
        
        // get event score
        eventMapFactors[ idx ] = computeModularityScore(idx[1], idx[2], event_type);
        
        std::cout << "";
    }
    
    return;
}
