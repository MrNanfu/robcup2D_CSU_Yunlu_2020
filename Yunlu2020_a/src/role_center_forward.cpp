// -*-c++-*-

/*
 *Copyright:

 Copyright (C) Hidehisa AKIYAMA

 This code is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3, or (at your option)
 any later version.

 This code is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this code; see the file COPYING.  If not, write to
 the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.

 *EndCopyright:
 */

/////////////////////////////////////////////////////////////////////

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "role_center_forward.h"
#include "role_offensive_half_move.h"
#include "role_center_back_move.h"
#include "strategy.h"
#include "bhv_chain_action.h"
#include "bhv_basic_offensive_kick.h"
#include "bhv_basic_move.h"
#include "bhv_attackers_move.h"

#include <rcsc/player/player_agent.h>
#include <rcsc/player/debug_client.h>
#include <rcsc/action/neck_turn_to_ball_or_scan.h>
#include <rcsc/common/logger.h>

using namespace rcsc;

const std::string RoleCenterForward::NAME( "CenterForward" );

/*-------------------------------------------------------------------*/
/*!

 */
namespace {
rcss::RegHolder role = SoccerRole::creators().autoReg( &RoleCenterForward::create,
                                                       RoleCenterForward::NAME );
}

/*-------------------------------------------------------------------*/
/*!

 */
bool
RoleCenterForward::execute( PlayerAgent * agent )
{


    bool kickable = agent->world().self().isKickable();
    if ( agent->world().existKickableTeammate()
         && agent->world().teammatesFromBall().front()->distFromBall()
         < agent->world().ball().distFromSelf() )
    {
        kickable = false;
    }

    if ( kickable )
    {
        doKick( agent );
    }
    else
    {
        doMove( agent );
    }

    return true;
}

/*-------------------------------------------------------------------*/
/*!

 */
void
RoleCenterForward::doKick( PlayerAgent * agent )
{

    const WorldModel & wm = agent->world();
    rcsc:Vector2D home_pos = Strategy::i().getPosition(wm.self().unum());

    if ( Bhv_ChainAction().execute( agent ) )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": (execute) do chain action" );
        agent->debugClient().addMessage( "ChainAction" );
        return;
    }
    Bhv_BasicOffensiveKick().execute( agent );
}

/*-------------------------------------------------------------------*/
/*!

 */
void
RoleCenterForward::doMove( PlayerAgent * agent )
{

    //Bhv_BasicMove().execute( agent );
	//yily adds the following
	  const rcsc::Vector2D base_pos = agent->world().ball().pos();
	  const rcsc::WorldModel & wm = agent->world();
	  rcsc::Vector2D home_pos = Strategy::i().getPosition(wm.self().unum());
	  if (!home_pos.isValid())
	    home_pos.assign(0.0, 0.0);

	  switch (Strategy::get_ball_area( base_pos))
	  {
	  case Strategy::BA_DribbleAttack:
	    //roleoffensivehalfmove().doOffensiveMove(agent, home_pos);
		Bhv_AttackersMove(home_pos,true).AttackersMove( agent );
				    Bhv_AttackersMove(home_pos,true).finalshoot (agent );//2017.7 gai
	    break;
	  case Strategy::BA_OffMidField:
	        if ( agent->world().ball().pos().x > 25.0 )
	        {
	        	Bhv_AttackersMove(home_pos,true).doGoToCrossPoint( agent );
//	        	roleoffensivehalfmove().doGoToCrossPoint( agent, home_pos );
	        }
	        else
	        {
	        	Bhv_AttackersMove(home_pos,true).AttackersMove(agent);
	        }
	    //roleoffensivehalfmove().doOffensiveMove(agent, home_pos);
	    break;
	  case Strategy::BA_Cross:
		Bhv_AttackersMove(home_pos,true).doGoToCrossPoint( agent);
		 Bhv_AttackersMove(home_pos,true).finalshoot (agent );//2017.7 gai
//		roleoffensivehalfmove().doGoToCrossPoint( agent, home_pos );
		break;
	  case Strategy::BA_ShootChance:
		Bhv_AttackersMove(home_pos,true).doGoToCrossPoint( agent);
		    Bhv_AttackersMove(home_pos,true).finalshoot (agent );//2017.7 gai
//		roleoffensivehalfmove().doGoToCrossPoint( agent, home_pos );
		break;
	  default:
	    rcsc::dlog.addText(rcsc::Logger::ROLE, __FILE__": unknown ball area");
	    Bhv_BasicMove().execute(agent);
	    agent->setNeckAction(new rcsc::Neck_TurnToBallOrScan());

	    break;
	  }
}
