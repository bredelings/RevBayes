/**
 * @file
 * This file contains the declaration of RbString, which is
 * a REvBayes wrapper around a regular string.
 *
 * @brief Declaration of RbString
 *
 * (c) Copyright 2009-
 * @date Last modified: $Date$
 * @author The REvBayes development core team
 * @license GPL version 3
 *
 * $Id$
 */

#ifndef RbDnaAlignment_H
#define RbDnaAlignment_H

#include "RbCharacterMatrix.h"

#include <string>

class RbDumpState;


class RbDnaAlignment : public RbCharacterMatrix {

    public:

	        RbDnaAlignment(std::string fileName, std::string fileType);   
	        RbDnaAlignment(const RbDnaAlignment& a);
	       ~RbDnaAlignment(void);
	       
	    RbObject*               clone() const;                          //!< Copy
	    bool                    equals(const RbObject* obj) const;      //!< Equals comparison
	    const StringVector&     getClass() const;                       //!< Get class
	    void                    printValue(std::ostream& o) const;      //!< Print value (for user)
	    std::string             toString(void) const;                   //!< General info on object

	    // overloaded operators
	    RbObject&               operator=(const RbObject& o);
	    RbCharacterMatrix&      operator=(const RbCharacterMatrix& o);
	    RbDnaAlignment&         operator=(const RbDnaAlignment& o);
		char convertToChar(RbBitset* bs);
		char getNucleotideChar(int nucCode);
		void initState(char nuc, RbBitset* bs);

        //void                    dump(std::ostream& c);                   //!< Dump to ostream c
        //void                    resurrect(const RbDumpState& x);         //!< Resurrect from dumped state

    protected:

	private:

};

#endif

