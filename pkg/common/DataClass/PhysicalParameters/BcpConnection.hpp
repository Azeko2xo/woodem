/*************************************************************************
*  Copyright (C) 2008 by Vincent Richefeu                                *
*  vincent.richefeu@hmg.inpg.fr                                          *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/


#ifndef BCP_CONNECTION_HPP
#define BCP_CONNECTION_HPP

#include <yade/pkg-dem/SimpleViscoelasticBodyParameters.hpp>


class BcpConnection : public SimpleViscoelasticBodyParameters
{
	public :
		// parameters (no parameters)

		// state
		unsigned int id1, id2;

		BcpConnection();
		virtual ~BcpConnection();

/// Serializable
	protected :
		virtual void registerAttributes();
	REGISTER_CLASS_NAME(BcpConnection);
	REGISTER_BASE_CLASS_NAME(SimpleViscoelasticBodyParameters);

/// Indexable
	REGISTER_CLASS_INDEX(BcpConnection,SimpleViscoelasticBodyParameters);

};

REGISTER_SERIALIZABLE(BcpConnection,false);

#endif // BCP_CONNECTION_HPP

