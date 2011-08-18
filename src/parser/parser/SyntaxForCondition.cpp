/**
 * @file
 * This file contains the implementation of SyntaxForCondition, which is
 * used to hold formal argument specifications in the syntax tree.
 *
 * @brief Implementation of SyntaxForCondition
 *
 * (c) Copyright 2009- under GPL version 3
 * @date Last modified: $Date$
 * @author The RevBayes Development Core Team
 * @license GPL version 3
 *
 * $Id$
 */

#include "ConstantNode.h"
#include "Environment.h"
#include "RbException.h"
#include "RbNames.h"
#include "RbString.h"
#include "SyntaxForCondition.h"
#include "VectorNatural.h"

#include <cassert>
#include <sstream>


/** Standard constructor */
SyntaxForCondition::SyntaxForCondition(RbString* identifier, SyntaxElement* inExpr) : SyntaxElement(), varName(identifier), inExpression(inExpr), vector(NULL), nextElement(-1) {

    if ( inExpression == NULL ) {
        delete varName;
        throw RbException("The 'in' expression of for loop is empty");
    }
}


/** Deep copy constructor */
SyntaxForCondition::SyntaxForCondition(const SyntaxForCondition& x) : SyntaxElement(x) {

    varName                  = new RbString(*(x.varName));
    inExpression             = x.inExpression->clone();
    vector                   = NULL;
    nextElement              = -1;
}


/** Destructor deletes members */
SyntaxForCondition::~SyntaxForCondition() {
    
    delete varName;
    delete inExpression;
    if ( nextElement >= 0 )
        delete vector;
}


/** Assignment operator */
SyntaxForCondition& SyntaxForCondition::operator=(const SyntaxForCondition& x) {

    if (&x != this) {
    
        SyntaxElement::operator=(x);

        delete varName;
        delete inExpression;
        if ( nextElement >= 0 )
            delete vector;

        varName                  = new RbString(*(x.varName));
        inExpression             = x.inExpression->clone();
        vector                   = NULL;
        nextElement              = -1;
    }

    return *this;
}


/** Return brief info about object */
std::string SyntaxForCondition::briefInfo () const {

    std::ostringstream   o;

    o << "SyntaxForCondition: variable = '" << std::string(*varName);
    o << "', in expression = ";
    o << inExpression->briefInfo();

    return o.str();
}


/** Clone syntax element */
SyntaxElement* SyntaxForCondition::clone () const {

    return (SyntaxElement*)(new SyntaxForCondition(*this));
}


/** Finalize loop. */
void SyntaxForCondition::finalizeLoop(Environment* env) {

    if ( nextElement < 0 )
        return;

    vector->release();
    if (vector->isUnreferenced())
        delete vector;
    
    nextElement = -1;
}


/** Get next loop state */
bool SyntaxForCondition::getNextLoopState(Environment* env) {

    if ( nextElement < 0 )
        initializeLoop( env );
    
    if ( nextElement == static_cast<int>(vector->getLength()) ) {
        finalizeLoop( env );
        return false;
    }

    (*env)[ *varName ].getVariable()->setDagNode( new ConstantNode((RbLanguageObject*)vector->getElement( nextElement )) );
    nextElement++;

    return true;
}


/** Get semantic value (not applicable so return NULL) */
Variable* SyntaxForCondition::getContentAsVariable(Environment* env) const {

    return NULL;
}


/** Initialize loop state */
void SyntaxForCondition::initializeLoop(Environment* env) {

    assert ( nextElement < 0 );

    // Evaluate expression and check that we get a vector
    DAGNode *theNode = inExpression->getContentAsVariable(env)->getDagNodePtr();
    RbLanguageObject *theValue = theNode->getValue()->clone();

    // Check that it is a vector
    if ( theValue->isType( Container_name ) == false ) {
        if (theNode->isUnreferenced())
            delete theNode;             // this will also delete the value 
        throw ( RbException("The 'in' expression does not evaluate to a vector") );
    }
    vector = dynamic_cast<Container*>(theValue);
    vector->retain();

    // Initialize nextValue
    nextElement = 0;

    // Add loop variable to frame if it is not there already
    if (!env->existsVariable(*varName)) {
        env->addVariable( *varName, TypeSpec( vector->getElementType() ) );
    }
    
    // cleaning up
    if (theNode->isUnreferenced())
        delete theNode;             // this will also delete the value 
}


/** Print info about syntax element */
void SyntaxForCondition::print(std::ostream& o) const {

    o << "SyntaxForCondition:" << std::endl;
    o << "varName      = " << std::string(*varName) << std::endl;
    o << "inExpression = " << inExpression;
    o << std::endl;

    inExpression->print(o);
}

