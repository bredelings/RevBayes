#ifndef Move_ReversibleJumpSwitch_H
#define Move_ReversibleJumpSwitch_H

#include "RlMove.h"
#include "TypedDagNode.h"

#include <ostream>
#include <string>

namespace RevLanguage {
    
    /**
     * @brief Rev Wrapper of a reversible jump move between a constant value and 
     *  a random distributedly value.
     *
     * This class is the RevLanguage wrapper of ReversibleJumpMixtureProposal.
     *
     * @author The RevBayes Development Core Team (Sebastian Hoehna)
     * @copyright GPL version 3
     * @since 2014-08-18, version 1.0
     */
    template <class rlValueType>
    class Move_ReversibleJumpSwitch : public Move {
        
    public:
        
        Move_ReversibleJumpSwitch(void);                                                                                                            //!< Default constructor
        
        // Basic utility functions
        virtual Move_ReversibleJumpSwitch*          clone(void) const;                                                                              //!< Clone the object
        void                                        constructInternalObject(void);                                                                  //!< We construct the a new internal move.
        static const std::string&                   getClassType(void);                                                                             //!< Get Rev type
        static const TypeSpec&                      getClassTypeSpec(void);                                                                         //!< Get class type spec
        std::string                                 getMoveName(void) const;                                                                        //!< Get the name used for the constructor function in Rev.
        const MemberRules&                          getParameterRules(void) const;                                                                  //!< Get member rules (const)
        virtual const TypeSpec&                     getTypeSpec(void) const;                                                                        //!< Get language type of the object
        virtual void                                printValue(std::ostream& o) const;                                                              //!< Print value (for user)
        
    protected:
        
        void                                        setConstParameter(const std::string& name, const RevPtr<const RevVariable> &var);               //!< Set member variable
        
        RevPtr<const RevVariable>                   x;                                                                                              //!< The variable holding the real valued vector.
        
    };
    
}


#include "ArgumentRule.h"
#include "ArgumentRules.h"
#include "MetropolisHastingsMove.h"
#include "ReversibleJumpMixtureProposal.h"
#include "RbException.h"
#include "TypedDagNode.h"
#include "TypeSpec.h"


using namespace RevLanguage;


template <class rlValueType>
RevLanguage::Move_ReversibleJumpSwitch<rlValueType>::Move_ReversibleJumpSwitch() : Move()
{
    
}


/**
 * The clone function is a convenience function to create proper copies of inherited objected.
 * E.g. a.clone() will create a clone of the correct type even if 'a' is of derived type 'b'.
 *
 * \return A new copy of the process.
 */
template <class rlValueType>
RevLanguage::Move_ReversibleJumpSwitch<rlValueType>* Move_ReversibleJumpSwitch<rlValueType>::clone(void) const
{
    
    return new Move_ReversibleJumpSwitch<rlValueType>(*this);
}


template <class rlValueType>
void RevLanguage::Move_ReversibleJumpSwitch<rlValueType>::constructInternalObject( void )
{
    // we free the memory first
    delete value;
    
    // now allocate a new vector-scale move
    double w = static_cast<const RealPos &>( weight->getRevObject() ).getValue();
    RevBayesCore::TypedDagNode<typename rlValueType::valueType>* tmp = static_cast<const rlValueType &>( x->getRevObject() ).getDagNode();
    RevBayesCore::StochasticNode< typename rlValueType::valueType > *sn = static_cast<RevBayesCore::StochasticNode< typename rlValueType::valueType > *>( tmp );
    
    RevBayesCore::ReversibleJumpMixtureProposal< typename rlValueType::valueType > *p = new RevBayesCore::ReversibleJumpMixtureProposal< typename rlValueType::valueType >(sn);
    value = new RevBayesCore::MetropolisHastingsMove(p,w);

}


/** Get Rev type of object */
template <class rlValueType>
const std::string& RevLanguage::Move_ReversibleJumpSwitch<rlValueType>::getClassType(void)
{
    
    //    static std::string rev_type = "Move_ReversibleJumpSwitch<" + rlValueType::getClassType() + ">";
    static std::string rev_type = "Move_ReversibleJumpSwitch__" + rlValueType::getClassType();
    
    return rev_type;
}

/** Get class type spec describing type of object */
template <class rlValueType>
const RevLanguage::TypeSpec& RevLanguage::Move_ReversibleJumpSwitch<rlValueType>::getClassTypeSpec(void)
{
    
    static TypeSpec rev_type_spec = TypeSpec( getClassType(), new TypeSpec( Move::getClassTypeSpec() ) );
    
    return rev_type_spec;
}


/**
 * Get the Rev name for the constructor function.
 *
 * \return Rev name of constructor function.
 */
template <class rlValueType>
std::string RevLanguage::Move_ReversibleJumpSwitch<rlValueType>::getMoveName( void ) const
{
    // create a constructor function name variable that is the same for all instance of this class
    std::string c_name = "RJSwitch";
    
    return c_name;
}


/** Return member rules (no members) */
template <class rlValueType>
const RevLanguage::MemberRules& RevLanguage::Move_ReversibleJumpSwitch<rlValueType>::getParameterRules(void) const
{
    
    static MemberRules move_member_rules;
    static bool rules_set = false;
    
    if ( !rules_set )
    {
        move_member_rules.push_back( new ArgumentRule( "x", rlValueType::getClassTypeSpec(), "The variable on which this move operates.", ArgumentRule::BY_REFERENCE, ArgumentRule::STOCHASTIC ) );
        
        /* Inherit weight from Move, put it after variable */
        const MemberRules& inheritedRules = Move::getParameterRules();
        move_member_rules.insert( move_member_rules.end(), inheritedRules.begin(), inheritedRules.end() );
        
        rules_set = true;
    }
    
    return move_member_rules;
}

/** Get type spec */
template <class rlValueType>
const RevLanguage::TypeSpec& RevLanguage::Move_ReversibleJumpSwitch<rlValueType>::getTypeSpec( void ) const
{
    
    static TypeSpec type_spec = getClassTypeSpec();
    
    return type_spec;
}


/** Get type spec */
template <class rlValueType>
void RevLanguage::Move_ReversibleJumpSwitch<rlValueType>::printValue(std::ostream &o) const
{
    
    o << "Move_ReversibleJumpSwitch(";
    if (x != NULL)
    {
        o << x->getName();
    }
    else
    {
        o << "?";
    }
    o << ")";
}


/** Set a member variable */
template <class rlValueType>
void RevLanguage::Move_ReversibleJumpSwitch<rlValueType>::setConstParameter(const std::string& name, const RevPtr<const RevVariable> &var) {
    
    if ( name == "x" )
    {
        x = var;
    }
    else
    {
        Move::setConstParameter(name, var);
    }
}


#endif
