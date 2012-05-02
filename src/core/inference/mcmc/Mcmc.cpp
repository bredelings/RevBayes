/**
 * @file
 * This file contains the implementation of Mcmc, which is used to hold
 * information about and run an mcmc analysis.
 *
 * @brief Implementation of Mcmc
 *
 * (c) Copyright 2009- under GPL version 3
 * @date Last modified: $Date$
 * @author The RevBayes Development Core Team
 * @license GPL version 3
 * @version 1.0
 * @since 2009-08-27, version 1.0
 * @interface Mcmc
 * @package distributions
 *
 * $Id$
 */

#include "ConstantInferenceNode.h"
#include "DagNodeContainer.h"
#include "Integer.h"
#include "InferenceDagNode.h"
#include "InferenceMove.h"
#include "InferenceMonitor.h"
#include "Mcmc.h"
#include "MemberFunction.h"
#include "Model.h"
#include "FileMonitor.h"
#include "Move.h"
#include "Natural.h"
#include "ParserMove.h"
#include "ParserMonitor.h"
#include "RandomNumberFactory.h"
#include "RandomNumberGenerator.h"
#include "RbException.h"
#include "RbNullObject.h"
#include "RangeRule.h"
#include "RbUtil.h"
#include "RbString.h"
#include "RbVector.h"
#include "StochasticInferenceNode.h"
#include "ValueRule.h"
#include "VariableNode.h"
#include "Workspace.h"

#include <cmath>
#include <fstream>
#include <sstream>
#include <string>
#include <typeinfo>



/** Constructor passes member rules and method inits to base class */
Mcmc::Mcmc(void) : MemberObject( getMemberRules() ) {
}

/** Copy constructor */
Mcmc::Mcmc(const Mcmc &x) : MemberObject(x), model( x.model ) { 
    
    extractDagNodesFromModel( model );
    
}


/*
 * Add a move and exchange the DAG nodes.
 */
void Mcmc::addMove( const DAGNode *m ) {
    
    // test whether var is a DagNodeContainer
    if ( m != NULL && m->getValue().isTypeSpec( DagNodeContainer::getClassTypeSpec() ) ) {
        const RbObject& objPtr = m->getValue();
        const DagNodeContainer& container = static_cast<const DagNodeContainer&>( objPtr );
        for (size_t i = 0; i < container.size(); ++i) {
            const RbObject& elemPtr = container.getElement(i);
            addMove(static_cast<const VariableSlot&>( elemPtr ).getVariable().getDagNode() );
        }
    }
    else {
        // first, we cast the value to a parser move
        const ParserMove &move = static_cast<const ParserMove&>( m->getValue() );
        
        // we need to extract the lean move
        InferenceMove* leanMove = move.getLeanMove()->clone();
        
        // create a named DAG node map
        std::map<std::string, InferenceDagNode *> nodesMap;
        for (std::vector<InferenceDagNode *>::iterator i = dagNodes.begin(); i != dagNodes.end(); ++i) {
            const std::string n = (*i)->getName();
            if ( n != "" ) {
                nodesMap.insert( std::pair<std::string, InferenceDagNode *>(n, *i) );
            }
        }
        
        // replace the DAG nodes of the move to point to our cloned DAG nodes
        const std::vector<RbConstDagNodePtr> orgNodes = move.getMoveArgumgents();
        std::vector<StochasticInferenceNode*> clonedNodes;
        for (std::vector<RbConstDagNodePtr>::const_iterator i = orgNodes.begin(); i != orgNodes.end(); ++i) {
            const DAGNode *orgNode = *i;
            const std::map<std::string, InferenceDagNode*>::const_iterator& it = nodesMap.find( orgNode->getName() );
            if ( it == nodesMap.end() ) {
                std::cerr << "Could not find the DAG node with name \"" << orgNode->getName() << "\" in the cloned model." << std::endl;
            }
            else {
                InferenceDagNode* clonedNode = it->second;
                if ( typeid( *clonedNode ) != typeid( StochasticInferenceNode ) ) {
                    throw RbException("We do not support moves on non-stochastic nodes.");
                }
                clonedNodes.push_back( static_cast<StochasticInferenceNode *>( clonedNode ) );
            }
        }
        leanMove->setArguments( clonedNodes );
        
        // finally, add the move to our moves list
        moves.push_back( leanMove );
    }
}

/** Clone object */
Mcmc* Mcmc::clone(void) const {

    return new Mcmc(*this);
}


/** Map calls to member methods */
const RbLanguageObject& Mcmc::executeOperationSimple(const std::string& name, const std::vector<Argument>& args) {

    if (name == "run") {
        const RbLanguageObject& argument = args[0].getVariable().getValue();
        int n = static_cast<const Natural&>( argument ).getValue();
        run(n);
        return RbNullObject::getInstance();
    }
//    else if ( name == "getMonitors" ) {
//        return monitors;
//    }

    return MemberObject::executeOperationSimple( name, args );
}


/**
 * Clone the model and extract the DAG nodes, moves and monitors.
 *
 * First we clone the entire DAG, then we clone the moves and the monitors and set the new nodes appropriately.
 */
void Mcmc::extractDagNodesFromModel(const Model& source) {
    dagNodes = source.getLeanDagNodes();
    
}


/** Get class name of object */
const std::string& Mcmc::getClassName(void) { 
    
    static std::string rbClassName = "MCMC";
    
	return rbClassName; 
}

/** Get class type spec describing type of object */
const TypeSpec& Mcmc::getClassTypeSpec(void) { 
    
    static TypeSpec rbClass = TypeSpec( getClassName(), new TypeSpec( MemberObject::getClassTypeSpec() ) );
    
	return rbClass; 
}

/** Get type spec */
const TypeSpec& Mcmc::getTypeSpec( void ) const {
    
    static TypeSpec typeSpec = getClassTypeSpec();
    
    return typeSpec;
}


/** Get member rules */
const MemberRules& Mcmc::getMemberRules(void) const {

    static MemberRules memberRules = ArgumentRules();
    static bool        rulesSet = false;

    if (!rulesSet) {

        memberRules.push_back( new ValueRule ( "model"    , Model::getClassTypeSpec()    ) );
        memberRules.push_back( new ValueRule( "moves"     , RbObject::getClassTypeSpec() ) );
        //        memberRules.push_back( new ValueRule( "monitors"  , ParserMonitor::getClassTypeSpec() ) );

        rulesSet = true;
    }

    return memberRules;
}


/** Get methods */
const MethodTable& Mcmc::getMethods(void) const {

    static MethodTable methods = MethodTable();
    static bool          methodsSet = false;

    if (!methodsSet) {
        
        // method "run" for "n" generations
        ArgumentRules* updateArgRules = new ArgumentRules();
        updateArgRules->push_back( new ValueRule( "generations", Natural::getClassTypeSpec()     ) );
        methods.addFunction("run", new MemberFunction( RbVoid_name, updateArgRules) );
        
//        // get Monitors
//        ArgumentRules* getMonitorsRules = new ArgumentRules();
//        methods.addFunction("getMonitors", new MemberFunction( RbVector<Monitor>::getClassTypeSpec() , getMonitorsRules) );

        methods.setParentTable( &MemberObject::getMethods() );
        methodsSet = true;
    }

    return methods;
}


/** Allow only constant member variables */
void Mcmc::setMemberVariable(const std::string& name, const Variable* var) {

    if ( name == "model" ) {
        model = static_cast<const Model&>( var->getValue() );
        extractDagNodesFromModel( model );
    }
    else if ( name == "moves" ) {
        addMove( var->getDagNode() );
        
    }
    else if ( name == "monitors" ) {
        ParserMonitor* move = static_cast<ParserMonitor*>( var->getValue().clone() );
        
        // exchange the DAG nodes in the move to point to our copied DAG nodes
//        monitor->exchangeDagNode();
        
        // add
//        monitors.push_back( monitor->getLeanMove() );
        
    }
    else {
        MemberObject::setMemberVariable(name, var);
    }
}


/** Run the mcmc chain */
void Mcmc::run(size_t ngen) {

    std::cerr << "Initializing mcmc chain ..." << std::endl;

    /* Get the chain settings */
    std::cerr << "Getting the chain settings ..." << std::endl;

    RandomNumberGenerator* rng        = GLOBAL_RNG;

    /* Open the output file and print headers */
    std::cerr << "Opening file and printing headers ..." << std::endl;
    for (size_t i=0; i<monitors.size(); i++) {
        // get the monitor
//        if (monitors[i].isTypeSpec( FileMonitor::getClassTypeSpec() ) ) {
//            
//            FileMonitor& theMonitor = static_cast<FileMonitor&>( monitors[i] );
//            
//            // open the file stream for the monitor
//            theMonitor.openStream();
//            
//            // print the header information
//            theMonitor.printHeader();
//        }
    }


    /* Get initial lnProbability of model */
    
    // first we touch all nodes so that the likelihood is dirty
    for (std::vector<InferenceDagNode *>::iterator i=dagNodes.begin(); i!=dagNodes.end(); i++) {
        (*i)->touch();
    }
    
    double lnProbability = 0.0;
    // just a debug counter
    size_t numSummedOver = 0;
    size_t numEliminated = 0;
    std::vector<double> initProb;
    for (std::vector<InferenceDagNode *>::iterator i=dagNodes.begin(); i!=dagNodes.end(); i++) {
        InferenceDagNode* node = (*i);
        if ( typeid(*node) == typeid(StochasticInferenceNode) ) {
            StochasticInferenceNode* stochNode = dynamic_cast<StochasticInferenceNode*>( node );
            // if this node is eliminated, then we do not need to add its log-likelihood
            if ( stochNode->isNotInstantiated() ) {
                
                // test whether the factor root is NULL
                // if so, we need to construct the sum-product sequence for this node
                if ( stochNode->getFactorRoot() == NULL ) {
                    stochNode->constructFactor();
                }
                
                // only if this node is actually the factor root we add the log-likelihood
                if ( stochNode->getFactorRoot() == stochNode) {
                    
                    double lnProb = stochNode->calculateLnProbability();
                    lnProbability += lnProb;
                }
                
                if ( stochNode->isSummedOver() ) {
                    numSummedOver++;
                }
                else {
                    numEliminated++;
                }
                
            }
            else {
                
                // if one of my parents is eliminated, then my likelihood shouldn't be added either
                bool eliminated = false;
                for (std::set<InferenceDagNode *>::iterator j = stochNode->getParents().begin(); j != stochNode->getParents().end(); j++) {
    
                    if ( (*j)->isNotInstantiated() ) {
                        eliminated = true;
                        numEliminated++;
                        
                        // we don't need to check the other parents
                        break;
                    }
                }
                
                // since this node is neither eliminated nor one of its parents, we add the log-likelihood
                if ( !eliminated ) {
                    
                    double lnProb = stochNode->calculateLnProbability();
                    lnProbability += lnProb;
                }
            }
//            initProb.push_back(lnProb);
        }
    }
    
    // now we keep all nodes so that the likelihood is stored
    for (std::vector<InferenceDagNode *>::iterator i=dagNodes.begin(); i!=dagNodes.end(); i++) {
        (*i)->keep();
    }
    
    std::cerr << "Number eliminated nodes = " << numEliminated << std::endl;
    std::cerr << "Number summedOver nodes = " << numSummedOver << std::endl;
    std::cerr << "Initial lnProbability = " << lnProbability << std::endl;

    /* Run the chain */
    std::cerr << "Running the chain ..." << std::endl;

    std::cout << std::endl;
    std::cout << "Gen\tlnProbability" << std::endl;
    
    /* Monitor */
    for (size_t i=0; i<monitors.size(); i++) {
        monitors[i]->monitor(0);
    }

    for (unsigned int gen=1; gen<=ngen; gen++) {

        for (size_t i=0; i<moves.size(); i++) {
            /* Get the move */
            InferenceMove* theMove = moves[i];

            /* Propose a new value */
            double lnProbabilityRatio;
            double lnHastingsRatio = theMove->perform(lnProbabilityRatio);
            // QUESTION: How to Gibbs samplers by-pass the accept-reject?
            /* Calculate acceptance ratio */
            double lnR = lnProbabilityRatio + lnHastingsRatio;
            double r;
            if (lnR > 0.0)
                r = 1.0;
            else if (lnR < -300.0)
                r = 0.0;
            else
                r = exp(lnR);
    
            /* Accept or reject the move */
            double u = rng->uniform01(); // TODO No need to draw rng if lnR > 0.0x
            if (u < r) {
                theMove->accept();
                lnProbability += lnProbabilityRatio;
            }
            else {
                theMove->reject();
            }

#ifdef DEBUG_MCMC
            /* Assert that the probability calculation shortcuts work */
            double curLnProb = 0.0;
            std::vector<double> lnRatio;
            for (std::vector<DAGNode*>::iterator i=dagNodes.begin(); i!=dagNodes.end(); i++) {
                if ((*i)->isTypeSpec(StochasticNode::getClassTypeSpec())) {
                    StochasticNode* stochNode = dynamic_cast<StochasticNode*>( *i );
                    double lnProb = stochNode->calculateLnProbability();
                    curLnProb += lnProb;
                    //                lnRatio.push_back(initProb[stochNode-dagNodes.begin()] - lnProb);
                }
            }
            if (fabs(lnProbability - curLnProb) > 1E-8)
                throw RbException("Error in ln probability calculation shortcuts");
#endif
        }

        /* Monitor */
        for (size_t i=0; i<monitors.size(); i++) {
            monitors[i]->monitor(gen);
        }

        /* Print to screen */
        std::cout << gen << "\t" << lnProbability << std::endl;

    }

    std::cerr << "Finished chain" << std::endl;

    std::cout << std::endl;
}

