/**
 * @file
 * This file contains the implementation of VariableSlot, which is
 * used to manage variables in frames and processed argument lists.
 *
 * @brief Implementation of VariableSlot
 *
 * (c) Copyright 2009- under GPL version 3
 * @date Last modified: $Date$
 * @author The RevBayes Development Core Team
 * @license GPL version 3
 * @version 1.0
 * @since 2009-11-17, version 1.0
 *
 * $Id$
 */

#include "ConstantNode.h"
#include "Container.h"
#include "DAGNode.h"
#include "MemberObject.h"
#include "RbException.h"
#include "RbNames.h"
#include "RbObject.h"
#include "RbOptions.h"
#include "TypeSpec.h"
#include "Variable.h"
#include "VariableNode.h"
#include "VariableSlot.h"
#include "VectorNatural.h"
#include "Workspace.h"

#include <cassert>


/** Constructor of filled slot with type specification. */
VariableSlot::VariableSlot(const std::string &lbl, const TypeSpec& typeSp, Variable *var) : RbInternal(), varTypeSpec(typeSp), label(lbl) {
    
    variable = var;
    variable->retain();
    
}

/** Constructor of filled slot with type specification. */
VariableSlot::VariableSlot(const std::string &lbl, Variable *var) : RbInternal() , varTypeSpec( TypeSpec(RbObject_name) ), label(lbl) {
    
    variable = var;
    variable->retain();
    
}


/** Constructor of empty slot based on type specification */
VariableSlot::VariableSlot(const std::string &lbl, const TypeSpec& typeSp) : RbInternal(), varTypeSpec(typeSp), label(lbl) {
    
    variable = NULL;
    
}


/** Copy constructor (shallow copy). */
VariableSlot::VariableSlot(const VariableSlot& x) : RbInternal(x), varTypeSpec(x.varTypeSpec), label(x.label) {
    
    if ( x.variable != NULL ) {
        variable = x.variable;
        variable->retain();
    }
}


/** Call a help function to remove the variable intelligently */
VariableSlot::~VariableSlot(void) {
    
    if (variable != NULL) {
        variable->release();
        if (variable->isUnreferenced()) {
            delete variable;
        }
    }
}


/** Assignment operator */
VariableSlot& VariableSlot::operator=(const VariableSlot& x) {
    
    if ( &x != this ) {
        
        // Check if assignment is possible
        if ( varTypeSpec != x.varTypeSpec )
            throw RbException ( "Invalid slot assignment: type difference" );
        
        // Copy the new variable
        if ( x.variable != NULL ) {
            variable = x.variable->clone();
        }
    }
    
    return (*this);
}


/** Clone slot and variable */
VariableSlot* VariableSlot::clone( void ) const {
    
    return new VariableSlot( *this );
}

/** Get class vector describing type of object */
const VectorString& VariableSlot::getClass() const {
    
    static VectorString rbClass = VectorString(VariableSlot_name) + RbInternal::getClass();
    return rbClass;
}


/** Get a const pointer to the dag node */
const DAGNode* VariableSlot::getDagNode( void ) const {
    
    if (variable == NULL) 
        return NULL;
    else
        return variable->getDagNode();
}


/** Get a reference to the slot variable */
DAGNode* VariableSlot::getDagNodePtr( void ) const {
    
    if (variable == NULL) 
        return NULL;
    else
        return variable->getDagNodePtr();
}


/** Get the value of the variable */
const RbLanguageObject* VariableSlot::getValue( void ) const {
    
    const RbLanguageObject *retVal = variable->getDagNodePtr()->getValue();
    
    // check the type and if we need conversion
    if (!retVal->isType(varTypeSpec)) {
        retVal = retVal->convertTo(varTypeSpec);
    }
    
    return retVal;
}


/** Is variable valid for the slot? Additional type checking here */
bool VariableSlot::isValidVariable( DAGNode* newVariable ) const {
    
    return true;    // No additional requirements here, but see MemberSlot
}



/* Print value of the slot variable */
void VariableSlot::printValue(std::ostream& o) const {
    
    if (variable == NULL)
        o << "NULL";
    else
        variable->printValue(o);
}


/** Set variable */
void VariableSlot::setVariable(Variable *var) {
    
    // change the old variable with the new variable in the parent and children
    
    // release the old variable
    if (variable != NULL) {
        variable->release();
        if (variable->isUnreferenced()) {
            delete variable;
        }
    }
    
    // set and retain the new variable
    variable = var;
    variable->retain();
    
}


/** Make sure we can print to stream using << operator */
std::ostream& operator<<(std::ostream& o, const VariableSlot& x) {
    
    o << "<" << x.getTypeSpec() << ">";
    if ( x.getLabel() != "" )
        o << " " << x.getLabel();
    o << " =";
    if ( x.getDagNode() == NULL )
        o << " NULL";
    else {
        o << " " << x.getDagNode()->briefInfo();
    }
    return o;
}

