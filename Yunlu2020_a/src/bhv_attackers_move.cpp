// -*-c++-*-

/*
 *Copyright:

 Copyright (C) Hidehisa AKIYAMA

 This code is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2, or (at your option)
 any later version.

 This code is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
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

#include "bhv_attackers_move.h"
#include "bhv_basic_tackle.h"
#include "bhv_basic_move.h"     //以后要逐渐取代掉
#include "bhv_set_play_indirect_free_kick.h"
#include <rcsc/action/body_kick_one_step.h>
#include <rcsc/action/body_intercept2009.h>
#include <rcsc/action/basic_actions.h>
#include <rcsc/action/bhv_scan_field.h>
#include <rcsc/action/body_go_to_point.h>
#include <rcsc/action/neck_scan_field.h>
#include <rcsc/action/neck_turn_to_ball_or_scan.h>
#include <rcsc/action/neck_turn_to_goalie_or_scan.h>
#include <rcsc/action/neck_turn_to_low_conf_teammate.h>
#include "strategy.h"
#include <rcsc/player/debug_client.h>
#include <rcsc/player/intercept_table.h>
#include <rcsc/player/player_agent.h>
#include <rcsc/player/soccer_intention.h>
#include <rcsc/player/say_message_builder.h>
#include <rcsc/action/bhv_shoot2008.h>

#include <rcsc/common/audio_memory.h>
#include <rcsc/common/logger.h>
#include <rcsc/common/server_param.h>

//#define DEBUG2014

using namespace rcsc;

/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/*!
need to change:
    经常会几个人都去抢球
    跑位不太好
    中锋抢球范围不要太大
 */
bool
Bhv_AttackersMove::AttackersMove(PlayerAgent * agent)
{
  const WorldModel & wm = agent->world();

//  dlog.addText(Logger::TEAM,
//      __FILE__": AttackerOffensiveMove. target=(%.2f %.2f)", M_home_pos.x,
  //    M_home_pos.y);

  //------------------------------------------------------
  // tackle
  if (Bhv_BasicTackle(0.75, 90.0).execute(agent))
  {
    return true;;
  }
double opp_dist = 1000.0;
  //------------------------------------------------------
  int self_min = wm.interceptTable()->selfReachCycle();
  int mate_min = wm.interceptTable()->teammateReachCycle();
  int opp_min = wm.interceptTable()->opponentReachCycle();
 // dlog.addText(Logger::TEAM,
 //     __FILE__": execute. intercept cycle. self=%d, mate=%d, opp=%d", self_min,
 //     mate_min, opp_min);

  //------------------------------------------------------
   double team_dist = wm.getDistTeammateNearestTo( wm.ball().pos(),
                                    10 ) ;
  if ((wm.self().stamina() > ServerParam::i().staminaMax() * 0.6)
      && opp_min < 3
      && opp_min < mate_min - 2
      && (opp_min < self_min - 2 || opp_min == 0
          || (opp_min == 1 && self_min > 1))
      && wm.ball().pos().dist(M_home_pos) < 10.0
      && wm.ball().distFromSelf() < 15.0)
  {
    if (Body_ForestallBlock(agent))
    {
      agent->setNeckAction(new Neck_TurnToBall());
#ifdef DEBUG2014
      std::cerr << wm.time().cycle() << __FILE__ << wm.self().unum() << ": Body_ForestallBlock.\n";
#endif
      return true;
    }
  }

  //------------------------------------------------------
  if ((mate_min >= 2 && self_min <= 4)
      || (self_min <= mate_min + 1 && mate_min >= 4))
  {
 //   dlog.addText(Logger::TEAM, __FILE__": execute. get ball");
 //   agent->debugClient().addMessage("AttGetBall");
    Vector2D face_point(52.5, wm.self().pos().y);

    Body_Intercept2009(true, face_point).execute(agent);
    if (self_min == 4 && opp_min >= 2)
    {
      agent->setViewAction(new View_Wide());
    }
    else if (self_min == 3 && opp_min >= 2)
    {
      agent->setViewAction(new rcsc::View_Normal());
    }
    agent->setNeckAction(new Neck_TurnToBallOrScan());
#ifdef DEBUG2014
    std::cerr << wm.time().cycle() << __FILE__ << wm.self().unum() << ": Body_Intercept.\n";
#endif // DEBUG2014

    return true;
  }

  //------------------------------------------------------

  const Vector2D mate_trap_pos = wm.ball().inertiaPoint(mate_min);

  Vector2D target_point = M_home_pos;

  const double max_x = std::max(wm.offsideLineX(), mate_trap_pos.x);
//  dlog.addText(Logger::TEAM, __FILE__": max x = %.1f", max_x);

  if (wm.self().pos().x > max_x && wm.self().pos().x < 42.0
  //&& std::fabs( wm.self().pos().y - M_home_pos.y ) < 10.0
      )
  {
    target_point.y = wm.self().pos().y;
  }

  if (std::fabs(mate_trap_pos.y - M_home_pos.y) < 15.0
      || mate_trap_pos.x > max_x - 5.0)
  {
    if (target_point.x > max_x - 1.5)
    {
      target_point.x = std::min(M_home_pos.x, max_x - 1.5);
    }
  }
  else
  {
    if (target_point.x > max_x - 3.0)
    {
      target_point.x = std::min(M_home_pos.x, max_x - 3.0);
    }
  }
int term = (agent->world().time().cycle()/10) % 10 ;
if(wm.ball().pos().x>=-10){
        //---------------x等于0-10的时候加速向前推进--------------------
if(self_min >= mate_min + 6 &&wm.self().unum() == 11){
    if(wm.ball().pos().x>= -10 && wm.self().pos().x >= 0 && wm.self().pos().x <= 10 &&  team_dist <= 2 && opp_dist <3 &&wm.self().unum() == 11){
        if(term == 0&& wm.self().unum() == 11){
            target_point.x = wm.self().pos().x + 3 ;
            target_point.y = 0 ;
        }
        if(term == 1 &&wm.self().unum() == 11 ){
            target_point.x = wm.self().pos().x + 3 ;
                target_point.y = 0 ;
        }

        if(term == 2 &&wm.self().unum() == 11){
            target_point.x = wm.self().pos().x + 3 ;
                target_point.y = 0 ;
            }

        if(term == 3 &&wm.self().unum() == 11){
            target_point.x = wm.self().pos().x + 3 ;
            target_point.y = 0 ;
            }


        if(term == 4 &&wm.self().unum() == 11){
                target_point.x = wm.self().pos().x + 3 ;
                target_point.y = 0;
            }
        if(term == 5 && wm.self().unum() == 11){
            target_point.x = wm.self().pos().x + 3 ;
            target_point.y = 0 ;
        }
        if(term == 6 &&wm.self().unum() == 11 ){
            target_point.x = wm.self().pos().x + 3 ;
                target_point.y = 0 ;
        }

        if(term == 7 &&wm.self().unum() == 11){
            target_point.x = wm.self().pos().x + 3 ;
                target_point.y = 0 ;
            }

        if(term == 8 &&wm.self().unum() == 11){
            target_point.x = wm.self().pos().x + 3 ;
            target_point.y = 0 ;
            }


        if(term == 9 &&wm.self().unum() == 11){
                target_point.x = wm.self().pos().x + 3 ;
                target_point.y = 0;
            }
    }

    //---------------------------------------------------------------


    //---------------------x等于10-25时曲线推进，同时迎9、10号的球----------------------------

    if(wm.ball().pos().x>= 0 && wm.self().pos().x >= 10 && wm.self().pos().x <= 25 &&  team_dist <= 2 && opp_dist <7 &&wm.self().unum() == 11){
        if(wm.ball().pos().y <= 0 ){                                                                                                             //接9号球
            if(term == 0&&wm.self().unum() == 11){
                target_point.x = wm.self().pos().x + 0.9 ;
                target_point.y = wm.self().pos().y ;
            }
            if(term == 1 &&wm.self().unum() == 11 ){
                target_point.x = wm.self().pos().x +0.9;
                    target_point.y = wm.self().pos().y - 3;

            }

            if(term == 2 &&wm.self().unum() == 11){
                target_point.x = wm.self().pos().x + 1.8 ;
                target_point.y = wm.self().pos().y - 2.7 ;
                }

            if(term == 3 &&wm.self().unum() == 11){
                target_point.x = wm.self().pos().x + 3 ;
                target_point.y = wm.self().pos().y - 0.9 ;

                }


            if(term == 4 &&wm.self().unum() == 11){
                target_point.x = wm.self().pos().x + 3 ;
                target_point.y = wm.self().pos().y ;
                    }
            if(term == 5 &&wm.self().unum() == 11){
                target_point.x = wm.self().pos().x + 0.9 ;
                target_point.y = wm.self().pos().y ;
            }
            if(term == 6 &&wm.self().unum() == 11 ){
                target_point.x = wm.self().pos().x +0.9;
                    target_point.y = wm.self().pos().y - 3;

            }

            if(term == 7 &&wm.self().unum() == 11){
                target_point.x = wm.self().pos().x + 1.8 ;
                target_point.y = wm.self().pos().y - 2.7 ;
                }

            if(term == 8 &&wm.self().unum() == 11){
                target_point.x = wm.self().pos().x + 3 ;
                target_point.y = wm.self().pos().y - 0.9 ;

                }


            if(term == 9 &&wm.self().unum() == 11){
                target_point.x = wm.self().pos().x + 3 ;
                target_point.y = wm.self().pos().y ;
                    }
        }
        if(wm.ball().pos().y > 0 ){                                                                                                             //接10号球
            if(term == 0&&wm.self().unum() == 11){
                target_point.x = wm.self().pos().x + 0.9 ;
                target_point.y = wm.self().pos().y ;
                    }
            if(term == 1 &&wm.self().unum() == 11 ){
                target_point.x = wm.self().pos().x +0.9;
                    target_point.y = wm.self().pos().y + 3;
                    }

            if(term == 2 &&wm.self().unum() == 11){
                target_point.x = wm.self().pos().x + 1.8 ;
                target_point.y = wm.self().pos().y + 2.7 ;
                    }

            if(term == 3 &&wm.self().unum() == 11){
                target_point.x = wm.self().pos().x + 3 ;
                target_point.y = wm.self().pos().y + 0.9 ;
                    }


            if(term == 4 &&wm.self().unum() == 11){
                target_point.x = wm.self().pos().x + 3 ;
                target_point.y = wm.self().pos().y ;
                    }
            if(term == 5 &&wm.self().unum() == 11){
                target_point.x = wm.self().pos().x + 0.9 ;
                target_point.y = wm.self().pos().y ;
                    }
            if(term == 6 &&wm.self().unum() == 11 ){
                target_point.x = wm.self().pos().x +0.9;
                    target_point.y = wm.self().pos().y + 3;
                    }

            if(term == 7 &&wm.self().unum() == 11){
                target_point.x = wm.self().pos().x + 1.8 ;
                target_point.y = wm.self().pos().y + 2.7 ;
                    }

            if(term == 8 &&wm.self().unum() == 11){
                target_point.x = wm.self().pos().x + 3 ;
                target_point.y = wm.self().pos().y + 0.9 ;
                    }


            if(term == 9 &&wm.self().unum() == 11){
                target_point.x = wm.self().pos().x + 3 ;
                target_point.y = wm.self().pos().y ;
                    }
        }
    }


    //-------------------------------------------------------------------------------------


    //---------------------x等于25-40时曲线推进----------------------------


    if(wm.ball().pos().x>= 10 && wm.self().pos().x >= 25 && wm.self().pos().x <= 40 &&  team_dist <= 2 && opp_dist <7 &&wm.self().unum() == 11){
        if(term == 0&&wm.self().unum() == 11){
            target_point.x = wm.self().pos().x ;
            target_point.y = wm.self().pos().y + 3 ;
 }
        if(term == 1 &&wm.self().unum() == 11 ){
            target_point.x = wm.self().pos().x + 0.9 ;
                target_point.y = wm.self().pos().y - 3 ;
}

        if(term == 2 &&wm.self().unum() == 11){
            target_point.x = wm.self().pos().x - 0.9 ;
                target_point.y = wm.self().pos().y ;

            }

        if(term == 3 &&wm.self().unum() == 11){
            target_point.x = wm.self().pos().x ;
            target_point.y = wm.self().pos().y - 3 ;

            }


        if(term == 4 &&wm.self().unum() == 11){
                target_point.x = wm.self().pos().x + 0.9 ;
                target_point.y = wm.self().pos().y + 3 ;

            }
        if(term == 5 &&wm.self().unum() == 11){
            target_point.x = wm.self().pos().x ;
            target_point.y = wm.self().pos().y + 3 ;
 }
        if(term == 6 &&wm.self().unum() == 11 ){
            target_point.x = wm.self().pos().x + 0.9 ;
                target_point.y = wm.self().pos().y - 3 ;
}

        if(term == 7 &&wm.self().unum() == 11){
            target_point.x = wm.self().pos().x - 0.9 ;
                target_point.y = wm.self().pos().y ;

            }

        if(term == 8 &&wm.self().unum() == 11){
            target_point.x = wm.self().pos().x ;
            target_point.y = wm.self().pos().y - 3 ;

            }


        if(term == 9 &&wm.self().unum() == 11){
                target_point.x = wm.self().pos().x + 0.9 ;
                target_point.y = wm.self().pos().y + 3 ;

            }
    }



    //-----------------------------------------------------------------------


    //---------------------x等于40-50时曲线推进--------------------------------


    if(wm.ball().pos().x >= 25 && wm.self().pos().x >= 25 && wm.self().pos().x <= 40 &&  team_dist <= 2 && opp_dist <5 &&wm.self().unum() == 11){
        if(wm.ball().pos().y <= 0 && wm.self().pos().y >=-20 ){
            if(term == 0 &&wm.self().unum() == 11){                                              //球在上半区、同时要避免跑出禁区
                target_point.x = wm.self().pos().x + 3;
                target_point.y = wm.self().pos().y ;

            }
            if(term == 1 &&wm.self().unum() == 11 ){
                target_point.x = wm.self().pos().x - 0.9 ;
                    target_point.y = wm.self().pos().y - 0.9 ;

            }

            if(term == 2 &&wm.self().unum() == 11){
                target_point.x = wm.self().pos().x + 1.5 ;
                target_point.y = wm.self().pos().y + 3 ;

                }

            if(term == 3 &&wm.self().unum() == 11){
                target_point.x = wm.self().pos().x - 3;
                target_point.y = wm.self().pos().y - 2.4 ;

                }


            if(term == 4 &&wm.self().unum() == 11){
                target_point.x = wm.self().pos().x ;
                target_point.y = wm.self().pos().y - 0.9;

                }
            if(term == 5 &&wm.self().unum() == 11){                                              //球在上半区、同时要避免跑出禁区
                target_point.x = wm.self().pos().x + 3;
                target_point.y = wm.self().pos().y ;

            }
            if(term == 6 &&wm.self().unum() == 11 ){
                target_point.x = wm.self().pos().x - 0.9 ;
                    target_point.y = wm.self().pos().y - 0.9 ;

            }

            if(term == 7 &&wm.self().unum() == 11){
                target_point.x = wm.self().pos().x + 1.5 ;
                target_point.y = wm.self().pos().y + 3 ;

                }

            if(term == 8 &&wm.self().unum() == 11){
                target_point.x = wm.self().pos().x - 3;
                target_point.y = wm.self().pos().y - 2.4 ;

                }


            if(term == 9 &&wm.self().unum() == 11){
                target_point.x = wm.self().pos().x ;
                target_point.y = wm.self().pos().y - 0.9;

                }
        }
        if(wm.ball().pos().y <= 0 && wm.self().pos().y >=-20 ){
            if(term == 0 &&wm.self().unum() == 11){                                              //球在下半区、同时要避免跑出禁区
                target_point.x = wm.self().pos().x + 3;
                target_point.y = wm.self().pos().y ;

            }
            if(term == 1 &&wm.self().unum() == 11 ){
                target_point.x = wm.self().pos().x - 0.9 ;
                    target_point.y = wm.self().pos().y + 0.9 ;

            }

            if(term == 2 &&wm.self().unum() == 11){
                target_point.x = wm.self().pos().x + 1.5 ;
                target_point.y = wm.self().pos().y - 3 ;

                }

            if(term == 3 &&wm.self().unum() == 11){
                target_point.x = wm.self().pos().x - 3;
                target_point.y = wm.self().pos().y + 2.4 ;

                }


            if(term == 4 &&wm.self().unum() == 11){
                target_point.x = wm.self().pos().x ;
                target_point.y = wm.self().pos().y + 0.9;

                }
            if(term == 5 &&wm.self().unum() == 11){                                              //球在下半区、同时要避免跑出禁区
                target_point.x = wm.self().pos().x + 3;
                target_point.y = wm.self().pos().y ;

            }
            if(term == 6 &&wm.self().unum() == 11 ){
                target_point.x = wm.self().pos().x - 0.9 ;
                    target_point.y = wm.self().pos().y + 0.9 ;

            }

            if(term == 7 &&wm.self().unum() == 11){
                target_point.x = wm.self().pos().x + 1.5 ;
                target_point.y = wm.self().pos().y - 3 ;

                }

            if(term == 8 &&wm.self().unum() == 11){
                target_point.x = wm.self().pos().x - 3;
                target_point.y = wm.self().pos().y + 2.4 ;

                }


            if(term == 9 &&wm.self().unum() == 11){
                target_point.x = wm.self().pos().x ;
                target_point.y = wm.self().pos().y + 0.9;

                }
        }
    }
        double dash_power1 = getDashPower(agent, target_point)*2;
        double dist_thr1 = std::fabs(wm.ball().pos().x - wm.self().pos().x) * 0.2 + 0.25;
         rcsc::Body_GoToPoint( target_point, dist_thr1, dash_power1,
                                     5, // cycle
                                     false, // no back dash
                                     true, // save recovery
                                     30.0 // dir thr
                                     ).execute( agent );
}
    //---------------x等于0-10的时候加速向前推进--------------------
    if(self_min >= mate_min + 6 &&wm.self().unum() == 9 ){
    if(wm.ball().pos().x>= -10 && wm.self().pos().x >= 0 && wm.self().pos().x <= 10 && team_dist <= 2 && opp_dist <7 &&wm.self().unum() == 9){
        if(term == 0 &&wm.self().unum() == 9){
            target_point.x = wm.self().pos().x + 3 ;
            target_point.y = -25 ;

        }
        if(term == 1 &&wm.self().unum() == 9 ){
            target_point.x = wm.self().pos().x + 3 ;
                target_point.y = -25 ;

        }

        if(term == 2 &&wm.self().unum() == 9){
            target_point.x = wm.self().pos().x + 3 ;
                target_point.y = -25 ;

            }

        if(term == 3 &&wm.self().unum() == 9){
            target_point.x = wm.self().pos().x + 3 ;
            target_point.y = -25 ;

            }


        if(term == 4 &&wm.self().unum() == 9){
                target_point.x = wm.self().pos().x + 3 ;
                target_point.y = -25;

            }
        if(term == 5 &&wm.self().unum() == 9){
            target_point.x = wm.self().pos().x + 3 ;
            target_point.y = -25 ;

        }
        if(term == 6 &&wm.self().unum() == 9 ){
            target_point.x = wm.self().pos().x + 3 ;
                target_point.y = -25 ;

        }

        if(term == 7 &&wm.self().unum() == 9){
            target_point.x = wm.self().pos().x + 3 ;
                target_point.y = -25 ;

            }

        if(term == 8 &&wm.self().unum() == 9){
            target_point.x = wm.self().pos().x + 3 ;
            target_point.y = -25 ;

            }

        if(term == 9 &&wm.self().unum() == 9){
                target_point.x = wm.self().pos().x + 3 ;
                target_point.y = -25;

            }
    }
    //---------------------------------------------------------------


    //---------------------x等于10-25时曲线推进，同时迎11号的球----------------------------


    if(wm.ball().pos().x>= 0 && wm.self().pos().x >= 10 && wm.self().pos().x <= 25 &&  team_dist <= 2 && wm.self().pos().y <= 0 && opp_dist <7 &&wm.self().unum() == 9){
        if(wm.ball().pos().y < 0 && wm.self().pos().x < wm.ball().pos().x + 5){                                                                                                          //球在上半区域,和十号形成配合
            if(term == 0 &&wm.self().unum() == 9 ){
                target_point.x = wm.self().pos().x + 2.1 ;
                target_point.y = wm.self().pos().y - 0.9 ;

            }
            if(term == 1 &&wm.self().unum() == 9 ){
                target_point.x = wm.self().pos().x + 2.1 ;
                    target_point.y = wm.self().pos().y - 0.3 ;

            }

            if(term == 2 &&wm.self().unum() == 9){
                target_point.x = wm.self().pos().x + 0.6 ;
                target_point.y = wm.self().pos().y + 3 ;

                }

            if(term == 3 &&wm.self().unum() == 9){
                target_point.x = wm.self().pos().x + 3 ;
                target_point.y = wm.self().pos().y - 3;

                }


            if(term == 4 &&wm.self().unum() == 9){
                target_point.x = wm.self().pos().x + 3 ;
                target_point.y = wm.self().pos().y + 0.9 ;

                }
            if(term == 5 &&wm.self().unum() == 9 ){
                target_point.x = wm.self().pos().x + 2.1 ;
                target_point.y = wm.self().pos().y - 0.9 ;

            }
            if(term == 6 &&wm.self().unum() == 9 ){
                target_point.x = wm.self().pos().x + 2.1 ;
                    target_point.y = wm.self().pos().y - 0.3 ;

            }

            if(term == 7 &&wm.self().unum() == 9){
                target_point.x = wm.self().pos().x + 0.6 ;
                target_point.y = wm.self().pos().y + 3 ;

                }

            if(term == 8 &&wm.self().unum() == 9){
                target_point.x = wm.self().pos().x + 3 ;
                target_point.y = wm.self().pos().y - 3;

                }


            if(term == 9 &&wm.self().unum() == 9){
                target_point.x = wm.self().pos().x + 3 ;
                target_point.y = wm.self().pos().y + 0.9 ;

                }
        }
        if(wm.ball().pos().y < 0 && wm.ball().pos().x + 5 < wm.self().pos().x){                                            //接10号球
            if(term == 0 &&wm.self().unum() == 9){
                target_point.x = wm.self().pos().x ;
                target_point.y = wm.self().pos().y + 0.9 ;

            }
            if(term == 1 &&wm.self().unum() == 9 ){
                target_point.x = wm.self().pos().x + 0.9 ;
                    target_point.y = wm.self().pos().y ;

            }

            if(term == 2 &&wm.self().unum() == 9){
                target_point.x = wm.self().pos().x + 0.9 ;
                target_point.y = wm.self().pos().y - 2.1 ;

                }

            if(term == 3 &&wm.self().unum() == 9){
                target_point.x = wm.self().pos().x + 0.3 ;
                target_point.y = wm.self().pos().y - 0.9 ;

                }


            if(term == 4 &&wm.self().unum() == 9){
                target_point.x = wm.self().pos().x + 0.3 ;
                target_point.y = wm.self().pos().y + 0.9;

                }
            if(term == 5 &&wm.self().unum() == 9){
                target_point.x = wm.self().pos().x ;
                target_point.y = wm.self().pos().y + 0.9 ;

            }
            if(term == 6 &&wm.self().unum() == 9 ){
                target_point.x = wm.self().pos().x + 0.9 ;
                    target_point.y = wm.self().pos().y ;

            }

            if(term == 7 &&wm.self().unum() == 9){
                target_point.x = wm.self().pos().x + 0.9 ;
                target_point.y = wm.self().pos().y - 2.1 ;

                }

            if(term == 8 &&wm.self().unum() == 9){
                target_point.x = wm.self().pos().x + 0.3 ;
                target_point.y = wm.self().pos().y - 0.9 ;

                }


            if(term == 9 &&wm.self().unum() == 9){
                target_point.x = wm.self().pos().x + 0.3 ;
                target_point.y = wm.self().pos().y + 0.9;

                }
        }
        if(wm.ball().pos().y > 0 && wm.self().pos().x < wm.ball().pos().x + 5){                                            //接10号球
            if(term == 0 &&wm.self().unum() == 9){
                target_point.x = wm.self().pos().x + 3;
                target_point.y = wm.self().pos().y ;

            }
            if(term == 1 &&wm.self().unum() == 9 ){
                target_point.x = wm.self().pos().x + 3;
                    target_point.y = wm.self().pos().y + 0.9;

            }

            if(term == 2 &&wm.self().unum() == 9){
                target_point.x = wm.self().pos().x + 3 ;
                target_point.y = wm.self().pos().y + 3 ;

                }

            if(term == 3 &&wm.self().unum() == 9){
                target_point.x = wm.self().pos().x + 1.5 ;
                target_point.y = wm.self().pos().y + 3 ;

                }


            if(term == 4 &&wm.self().unum() == 9){
                target_point.x = wm.self().pos().x + 0.9 ;
                target_point.y = wm.self().pos().y + 3 ;

                }
            if(term == 5 &&wm.self().unum() == 9){
                target_point.x = wm.self().pos().x + 3;
                target_point.y = wm.self().pos().y ;

            }
            if(term == 6 &&wm.self().unum() == 9 ){
                target_point.x = wm.self().pos().x + 3;
                    target_point.y = wm.self().pos().y + 0.9;

            }

            if(term == 7 &&wm.self().unum() == 9){
                target_point.x = wm.self().pos().x + 3 ;
                target_point.y = wm.self().pos().y + 3 ;

                }

            if(term == 8 &&wm.self().unum() == 9){
                target_point.x = wm.self().pos().x + 1.5 ;
                target_point.y = wm.self().pos().y + 3 ;

                }


            if(term == 9 &&wm.self().unum() == 9){
                target_point.x = wm.self().pos().x + 0.9 ;
                target_point.y = wm.self().pos().y + 3 ;

                }
        }
    }


    //-------------------------------------------------------------------------------------


    //---------------------x等于25-35时曲线推进----------------------------


    if(wm.ball().pos().x>= 10 && wm.self().pos().x >= 25 && wm.self().pos().x <= 35 &&  team_dist <= 2 && wm.self().pos().y < 0 && opp_dist <7 &&wm.self().unum() == 9){
        if(term == 0 &&wm.self().unum() == 9){
            target_point.x = wm.self().pos().x + 0.9 ;
            target_point.y = wm.self().pos().y - 3 ;

        }
        if(term == 1 &&wm.self().unum() == 9 ){
            target_point.x = wm.self().pos().x + 3 ;
                target_point.y = wm.self().pos().y - 0.9 ;

        }

        if(term == 2 &&wm.self().unum() == 9){
            target_point.x = wm.self().pos().x + 3 ;
                target_point.y = wm.self().pos().y + 3 ;

            }

        if(term == 3 &&wm.self().unum() == 9){
            target_point.x = wm.self().pos().x + 3 ;
            target_point.y = wm.self().pos().y - 0.9 ;

            }


        if(term == 4 &&wm.self().unum() == 9){
                target_point.x = wm.self().pos().x + 3 ;
                target_point.y = wm.self().pos().y ;

            }
        if(term == 5 &&wm.self().unum() == 9){
            target_point.x = wm.self().pos().x + 0.9 ;
            target_point.y = wm.self().pos().y - 3 ;

        }
        if(term == 6 &&wm.self().unum() == 9 ){
            target_point.x = wm.self().pos().x + 3 ;
                target_point.y = wm.self().pos().y - 0.9 ;

        }

        if(term == 7 &&wm.self().unum() == 9){
            target_point.x = wm.self().pos().x + 3 ;
                target_point.y = wm.self().pos().y + 3 ;

            }

        if(term == 8 &&wm.self().unum() == 9){
            target_point.x = wm.self().pos().x + 3 ;
            target_point.y = wm.self().pos().y - 0.9 ;

            }


        if(term == 9 &&wm.self().unum() == 9){
                target_point.x = wm.self().pos().x + 3 ;
                target_point.y = wm.self().pos().y ;

            }
    }



    //-----------------------------------------------------------------------


    //---------------------x等于35-50时曲线推进--------------------------------


    if(wm.ball().pos().x >= 25 && wm.self().pos().x >= 35 && wm.self().pos().x <= 50 &&  team_dist <= 2 && wm.self().pos().y < 0 && opp_dist <5 &&wm.self().unum() == 9){
        if(wm.ball().pos().y <= 0 && wm.self().pos().y >=-20 ){
            if(term == 0 &&wm.self().unum() == 9){                                              //球在上半区、同时要避免跑出禁区
                target_point.x = wm.self().pos().x + 0.9;
                target_point.y = wm.self().pos().y - 0.9;

            }
            if(term == 1 &&wm.self().unum() == 9 ){
                target_point.x = wm.self().pos().x - 3 ;
                    target_point.y = wm.self().pos().y + 1.5 ;

            }

            if(term == 2 &&wm.self().unum() == 9){
                target_point.x = wm.self().pos().x - 0.9 ;
                target_point.y = wm.self().pos().y + 3 ;

                }

            if(term == 3 &&wm.self().unum() == 9){
                target_point.x = wm.self().pos().x + 1.5 ;
                target_point.y = wm.self().pos().y + 1.5 ;

                }


            if(term == 4 &&wm.self().unum() == 9){
                target_point.x = wm.self().pos().x - 0.3;
                target_point.y = wm.self().pos().y - 0.9;

                }
            if(term == 5 &&wm.self().unum() == 9){                                              //球在上半区、同时要避免跑出禁区
                target_point.x = wm.self().pos().x + 0.9;
                target_point.y = wm.self().pos().y - 0.9;

            }
            if(term == 6 &&wm.self().unum() == 9 ){
                target_point.x = wm.self().pos().x - 3 ;
                    target_point.y = wm.self().pos().y + 1.5 ;

            }

            if(term == 7 &&wm.self().unum() == 9){
                target_point.x = wm.self().pos().x - 0.9 ;
                target_point.y = wm.self().pos().y + 3 ;

                }

            if(term == 8 &&wm.self().unum() == 9){
                target_point.x = wm.self().pos().x + 1.5 ;
                target_point.y = wm.self().pos().y + 1.5 ;

                }


            if(term == 9 &&wm.self().unum() == 9){
                target_point.x = wm.self().pos().x - 0.3;
                target_point.y = wm.self().pos().y - 0.9;

                }
        }

                }
double dist_thr1 = std::fabs(wm.ball().pos().x - wm.self().pos().x) * 0.2
      + 0.25;
        double dash_power1 = getDashPower(agent, target_point)*2;
         rcsc::Body_GoToPoint( target_point, dist_thr1, dash_power1,
                                     5, // cycle
                                     false, // no back dash
                                     true, // save recovery
                                     30.0 // dir thr
                                     ).execute( agent );
 }
    //---------------x等于0-10的时候加速向前推进--------------------
    if(self_min >= mate_min + 6 && wm.self().unum() == 10 ){
    if(wm.ball().pos().x>= -10 && wm.self().pos().x >= 0 && wm.self().pos().x <= 10 && team_dist <= 2 && wm.self().pos().y >= 0 && opp_dist < 7 && wm.self().unum() == 10){
        if(term == 0 && wm.self().unum() == 10){
            target_point.x = wm.self().pos().x + 3 ;
            target_point.y = 25 ;

        }
        if(term == 1 &&wm.self().unum() == 10 ){
            target_point.x = wm.self().pos().x + 3 ;
                target_point.y = 25 ;

        }

        if(term == 2 &&wm.self().unum() == 10){
            target_point.x = wm.self().pos().x + 3 ;
                target_point.y = 25 ;

            }

        if(term == 3 &&wm.self().unum() == 10){
            target_point.x = wm.self().pos().x + 3 ;
            target_point.y = 25 ;

            }


        if(term == 4 &&wm.self().unum() == 10){
                target_point.x = wm.self().pos().x + 3 ;
                target_point.y = 25;

            }
        if(term == 5 && wm.self().unum() == 10){
            target_point.x = wm.self().pos().x + 3 ;
            target_point.y = 25 ;

        }
        if(term == 6 &&wm.self().unum() == 10 ){
            target_point.x = wm.self().pos().x + 3 ;
                target_point.y = 25 ;

        }

        if(term == 7 &&wm.self().unum() == 10){
            target_point.x = wm.self().pos().x + 3 ;
                target_point.y = 25 ;

            }

        if(term == 8 &&wm.self().unum() == 10){
            target_point.x = wm.self().pos().x + 3 ;
            target_point.y = 25 ;

            }


        if(term == 9 &&wm.self().unum() == 10){
                target_point.x = wm.self().pos().x + 3 ;
                target_point.y = 25;

            }
    }
    //---------------------------------------------------------------


    //---------------------x等于10-25时曲线推进，同时迎11号的球----------------------------


    if(wm.ball().pos().x>= 0 && wm.self().pos().x >= 10 && wm.self().pos().x <= 25 && opp_dist < 7 &&  team_dist <= 2 && wm.self().pos().y >= 0 && wm.self().unum() == 10){
        if(wm.ball().pos().y >= 0 ){                                                                                                          //球在下半区域,和十一号形成配合
            if(term == 0 &&wm.self().unum() == 10 ){
                target_point.x = wm.self().pos().x + 3 ;
                target_point.y = wm.self().pos().y - 3 ;

            }
            if(term == 1 &&wm.self().unum() == 10 ){
                target_point.x = wm.self().pos().x + 3 ;
                    target_point.y = wm.self().pos().y + 3 ;

            }

            if(term == 2 &&wm.self().unum() == 10){
                target_point.x = wm.self().pos().x + 3 ;
                target_point.y = wm.self().pos().y - 0.9 ;

                }

            if(term == 3 &&wm.self().unum() == 10){
                target_point.x = wm.self().pos().x + 2.1 ;
                target_point.y = wm.self().pos().y + 0.9 ;

                }


            if(term == 4 &&wm.self().unum() == 10){
                target_point.x = wm.self().pos().x + 2.1 ;
                target_point.y = wm.self().pos().y + 0.3 ;

                }
            if(term == 5 &&wm.self().unum() == 10 ){
                target_point.x = wm.self().pos().x + 3 ;
                target_point.y = wm.self().pos().y - 3 ;

            }
            if(term == 6 &&wm.self().unum() == 10 ){
                target_point.x = wm.self().pos().x + 3 ;
                    target_point.y = wm.self().pos().y + 3 ;

            }

            if(term == 7 &&wm.self().unum() == 10){
                target_point.x = wm.self().pos().x + 3 ;
                target_point.y = wm.self().pos().y - 0.9 ;

                }

            if(term == 8 &&wm.self().unum() == 10){
                target_point.x = wm.self().pos().x + 2.1 ;
                target_point.y = wm.self().pos().y + 0.9 ;

                }


            if(term == 9 &&wm.self().unum() == 10){
                target_point.x = wm.self().pos().x + 2.1 ;
                target_point.y = wm.self().pos().y + 0.3 ;

                }
        }
        if(wm.ball().pos().y < 0 && wm.ball().pos().x + 5 < wm.self().pos().x){                                            //接11号球
            if(term == 0 &&wm.self().unum() == 10){
                target_point.x = wm.self().pos().x + 0.3 ;
                target_point.y = wm.self().pos().y + 0.9 ;

            }
            if(term == 1 &&wm.self().unum() == 10 ){
                target_point.x = wm.self().pos().x + 0.3;
                    target_point.y = wm.self().pos().y - 0.9 ;

            }

            if(term == 2 &&wm.self().unum() == 10 ){
                target_point.x = wm.self().pos().x ;
                target_point.y = wm.self().pos().y - 0.9 ;

                }

            if(term == 3 &&wm.self().unum() == 10 ){
                target_point.x = wm.self().pos().x + 0.9 ;
                target_point.y = wm.self().pos().y ;

                }


            if(term == 4 &&wm.self().unum() == 10 ){
                target_point.x = wm.self().pos().x + 0.9 ;
                target_point.y = wm.self().pos().y + 2.1 ;

                }
            if(term == 5 &&wm.self().unum() == 10){
                target_point.x = wm.self().pos().x + 0.3 ;
                target_point.y = wm.self().pos().y + 0.9 ;

            }
            if(term == 6 &&wm.self().unum() == 10 ){
                target_point.x = wm.self().pos().x + 0.3;
                    target_point.y = wm.self().pos().y - 0.9 ;

            }

            if(term == 7 &&wm.self().unum() == 10 ){
                target_point.x = wm.self().pos().x ;
                target_point.y = wm.self().pos().y - 0.9 ;

                }

            if(term == 8 &&wm.self().unum() == 10 ){
                target_point.x = wm.self().pos().x + 0.9 ;
                target_point.y = wm.self().pos().y ;

                }


            if(term == 9 &&wm.self().unum() == 10 ){
                target_point.x = wm.self().pos().x + 0.9 ;
                target_point.y = wm.self().pos().y + 2.1 ;

                }
        }
        if(wm.ball().pos().y < 0 && wm.self().pos().x < wm.ball().pos().x){                                            //接11号球
            if(term == 0 &&wm.self().unum() == 10){
                target_point.x = wm.self().pos().x + 3;
                target_point.y = wm.self().pos().y - 3;

            }
            if(term == 1 &&wm.self().unum() == 10 ){
                target_point.x = wm.self().pos().x + 1.5 ;
                    target_point.y = wm.self().pos().y - 3 ;

            }

            if(term == 2 &&wm.self().unum() == 10){
                target_point.x = wm.self().pos().x + 0.9 ;
                target_point.y = wm.self().pos().y - 3 ;

                }

            if(term == 3 &&wm.self().unum() == 10){
                target_point.x = wm.self().pos().x + 3 ;
                target_point.y = wm.self().pos().y ;

                }


            if(term == 4 &&wm.self().unum() == 10){
                target_point.x = wm.self().pos().x + 3 ;
                target_point.y = wm.self().pos().y - 0.9 ;

                }
            if(term == 5 &&wm.self().unum() == 10){
                target_point.x = wm.self().pos().x + 3;
                target_point.y = wm.self().pos().y - 3;

            }
            if(term == 6 &&wm.self().unum() == 10 ){
                target_point.x = wm.self().pos().x + 1.5 ;
                    target_point.y = wm.self().pos().y - 3 ;

            }

            if(term == 7 &&wm.self().unum() == 10){
                target_point.x = wm.self().pos().x + 0.9 ;
                target_point.y = wm.self().pos().y - 3 ;

                }

            if(term == 8 &&wm.self().unum() == 10){
                target_point.x = wm.self().pos().x + 3 ;
                target_point.y = wm.self().pos().y ;

                }


            if(term == 9 &&wm.self().unum() == 10){
                target_point.x = wm.self().pos().x + 3 ;
                target_point.y = wm.self().pos().y - 0.9 ;

                }
        }
    }


    //-------------------------------------------------------------------------------------


    //---------------------x等于25-35时曲线推进----------------------------


    if(wm.ball().pos().x>= 10 && wm.self().pos().x >= 25 && wm.self().pos().x <= 35 && opp_dist < 7 &&  team_dist <= 2 && wm.self().pos().y >= 0 && wm.self().unum() == 10){
        if(term == 0 &&wm.self().unum() == 10){
            target_point.x = wm.self().pos().x + 3 ;
            target_point.y = wm.self().pos().y - 3 ;

        }
        if(term == 1 &&wm.self().unum() == 10 ){
            target_point.x = wm.self().pos().x + 3 ;
                target_point.y = wm.self().pos().y + 0.9 ;

        }

        if(term == 2 &&wm.self().unum() == 10){
            target_point.x = wm.self().pos().x + 3 ;
                target_point.y = wm.self().pos().y ;

            }

        if(term == 3 &&wm.self().unum() == 10){
            target_point.x = wm.self().pos().x + 0.9 ;
            target_point.y = wm.self().pos().y + 3 ;

            }


        if(term == 4 &&wm.self().unum() == 10){
                target_point.x = wm.self().pos().x + 3 ;
                target_point.y = wm.self().pos().y + 0.9 ;

            }
        if(term == 5 &&wm.self().unum() == 10){
            target_point.x = wm.self().pos().x + 3 ;
            target_point.y = wm.self().pos().y - 3 ;

        }
        if(term == 6 &&wm.self().unum() == 10 ){
            target_point.x = wm.self().pos().x + 3 ;
                target_point.y = wm.self().pos().y + 0.9 ;

        }

        if(term == 7 &&wm.self().unum() == 10){
            target_point.x = wm.self().pos().x + 3 ;
                target_point.y = wm.self().pos().y ;

            }

        if(term == 8 &&wm.self().unum() == 10){
            target_point.x = wm.self().pos().x + 0.9 ;
            target_point.y = wm.self().pos().y + 3 ;

            }


        if(term == 9 &&wm.self().unum() == 10){
                target_point.x = wm.self().pos().x + 3 ;
                target_point.y = wm.self().pos().y + 0.9 ;

            }
    }



    //-----------------------------------------------------------------------


    //---------------------x等于35-50时曲线推进--------------------------------


    if(wm.ball().pos().x >= 25 && wm.self().pos().x >= 35 && wm.self().pos().x <= 50 && opp_dist < 5 &&  team_dist <= 2 && wm.self().pos().y >= 0 && wm.self().unum() == 10){
        if(wm.ball().pos().y >= 0 && wm.self().pos().y <= 20 ){
            if(term == 0 &&wm.self().unum() == 10){                                              //球在上半区、同时要避免跑出禁区
                target_point.x = wm.self().pos().x - 0.9;
                target_point.y = wm.self().pos().y - 3;

            }
            if(term == 1 &&wm.self().unum() == 10 ){
                target_point.x = wm.self().pos().x + 1.5 ;
                    target_point.y = wm.self().pos().y - 1.5 ;

            }

            if(term == 2 &&wm.self().unum() == 10){
                target_point.x = wm.self().pos().x - 0.3 ;
                target_point.y = wm.self().pos().y + 0.9 ;

                            }

            if(term == 3 &&wm.self().unum() == 10){
                target_point.x = wm.self().pos().x + 0.9 ;
                target_point.y = wm.self().pos().y + 0.9 ;

                }


            if(term == 4 &&wm.self().unum() == 10){
                target_point.x = wm.self().pos().x - 3;
                target_point.y = wm.self().pos().y - 1.5;

                }
            if(term == 5 &&wm.self().unum() == 10){                                              //球在上半区、同时要避免跑出禁区
                target_point.x = wm.self().pos().x - 0.9;
                target_point.y = wm.self().pos().y - 3;

            }
            if(term == 6 &&wm.self().unum() == 10 ){
                target_point.x = wm.self().pos().x + 1.5 ;
                    target_point.y = wm.self().pos().y - 1.5 ;

            }

            if(term == 7 &&wm.self().unum() == 10){
                target_point.x = wm.self().pos().x - 0.3 ;
                target_point.y = wm.self().pos().y + 0.9 ;

                            }

            if(term == 8 &&wm.self().unum() == 10){
                target_point.x = wm.self().pos().x + 0.9 ;
                target_point.y = wm.self().pos().y + 0.9 ;

                }


            if(term == 9 &&wm.self().unum() == 10){
                target_point.x = wm.self().pos().x - 3;
                target_point.y = wm.self().pos().y - 1.5;

                }

        }

    }
double dist_thr1 = std::fabs(wm.ball().pos().x - wm.self().pos().x) * 0.2
      + 0.25;
        double dash_power1 = getDashPower(agent, target_point)*2;
         rcsc::Body_GoToPoint( target_point, dist_thr1, dash_power1,
                                     5, // cycle
                                     false, // no back dash
                                     true, // save recovery
                                     30.0 // dir thr
                                     ).execute( agent );
}
  }

//2017.7  如果碰到包夹，就传出来
    Vector2D target = wm.self().pos();
   double ball_speed = wm.self().kickRate() * ServerParam::i().maxPower();
    rcsc::Vector2D goal_c = ServerParam::i().theirTeamGoalPos();
    double min_dist=100.0;
    const PlayerObject *  mate_nearst_to_me = wm.getTeammateNearestTo(target,10,&min_dist);
bool existoppblockball=0;
    int countOfOpps = 0;
int countOfOpps_mate=0;
   const PlayerPtrCont::const_iterator t_end = wm.opponentsFromSelf().end();
    for ( PlayerPtrCont::const_iterator t = wm.opponentsFromSelf().begin();
          t != t_end;
          ++t )
        {
            if ( (*t)->pos().dist(target)<3 && (*t)->pos().x > target.x)
            {
                countOfOpps++;
            }
            if ( (*t)->pos().dist( mate_nearst_to_me->pos())<=2)
            {
                countOfOpps_mate++;
            }
            if((*t)->pos().dist(mate_nearst_to_me->pos())+ (*t)->pos().dist(target)<=mate_nearst_to_me->pos().dist( target)+2)
            existoppblockball=1;
        }
        if(wm.existKickableTeammate()&&countOfOpps >= 2&&  countOfOpps_mate<1
       &&!AttackChance( agent )&&!existoppblockball)
    {
          Body_KickOneStep( mate_nearst_to_me->pos(), ball_speed).execute( agent );
    }




#if 1
  // 2008-04-23 akiyama
  if (mate_min >= 3 && wm.self().pos().dist2(target_point) < 5.0 * 5.0)
  {
    double opp_dist = 1000.0;
    const PlayerObject * opp = wm.getOpponentNearestTo(target_point, 10,
        &opp_dist);
    if (opp && opp_dist < 4.0 && std::fabs(opp->pos().y - target_point.y) < 2.0)
    {
//             target_point.y += ( target_point.y > opp->pos().y
//                                 ? +3.0
//                                 : -3.0 );
      target_point.y = (
          target_point.y > opp->pos().y ?
              opp->pos().y + 2.0 : opp->pos().y - 2.0 );
//      dlog.addText(Logger::TEAM,
 //         __FILE__": adjust target point to avvoid opponent. y=%.1f",
 //         target_point.y);
    }
  }
#endif
#if 1
  // 2008-04-28 akiyama
  if (mate_min < 3 && std::fabs(wm.self().pos().y - M_home_pos.y) < 3.0)
  {
    target_point.y = wm.self().pos().y * 0.9 + M_home_pos.y * 0.1;
    ;
 //   dlog.addText(Logger::TEAM,
  //      __FILE__": adjust target point to prepare receive. y=%.1f",
  //      target_point.y);
  }
#endif

  bool breakaway = false;
  bool intentional = false;

  if (wm.existKickableTeammate() || mate_min <= 5 || mate_min <= opp_min + 1)
  {
    if (wm.self().pos().x > max_x)
    {
      if (std::fabs(mate_trap_pos.y - target_point.y) < 4.0)
      {
        double abs_y = wm.ball().pos().absY();
        bool outer = (wm.self().pos().absY() > abs_y );
        if (abs_y > 25.0)
          target_point.y = (outer ? 30.0 : 20.0 );
        else if (abs_y > 20.0)
          target_point.y = (outer ? 25.0 : 15.0 );
        else if (abs_y > 15.0)
          target_point.y = (outer ? 20.0 : 10.0 );
        else if (abs_y > 10.0)
          target_point.y = (outer ? 15.0 : 5.0 );
        else if (abs_y > 5.0)
          target_point.y = (outer ? 10.0 : 0.0 );
        else
          target_point.y = (outer ? 5.0 : -5.0 );

        if (wm.self().pos().y < 0.0)
        {
          target_point.y *= -1.0;
        }

   //     agent->debugClient().addMessage("AvoidOffside");
   //     dlog.addText(Logger::TEAM,
   //         __FILE__": avoid offside possibility. new_target=(%.1f %.1f)",
   //         target_point.x, target_point.y);
      }
    }
    else if (M_forward_player
    //&& wm.audioMemory().dribbleTime() != wm.time()
        && mate_min <= 2 && opp_min >= 3
        && mate_trap_pos.x > wm.offsideLineX() - 10.0
        && wm.self().stamina() > ServerParam::i().staminaMax() * 0.8
        && 5.0 < wm.self().pos().x && wm.self().pos().x < 27.0
        && wm.self().pos().x > max_x - 7.0 && wm.self().pos().x < max_x - 1.0
        && wm.self().pos().x < mate_trap_pos.x + 10.0
        && (std::fabs(mate_trap_pos.y - wm.self().pos().y) < 8.0
            || (mate_trap_pos - wm.self().pos()).th().abs() < 110.0
        //|| ( mate_trap_pos.x > wm.offsideLineX() - 8.0
        //     && std::fabs( mate_trap_pos.y - wm.self().pos().y ) < 20.0 )
        ) && wm.self().pos().dist(mate_trap_pos) < 20.0
        && std::fabs(M_home_pos.y - wm.self().pos().y) < 15.0)
    {
      double x_diff = max_x - wm.self().pos().x;
      int dash_step = wm.self().playerType().cyclesToReachDistance(x_diff);
      if (mate_min < dash_step - 1)
      {
        target_point.x = std::min(wm.self().pos().x + 20.0, 50.0);
        target_point.y = wm.self().pos().y * 0.8 + mate_trap_pos.y * 0.2;
        if (target_point.absY() > 8.0
            && target_point.absY() > M_home_pos.absY())
        {
          target_point.y = M_home_pos.y;
        }
        intentional = true;
      }

 //     dlog.addText(Logger::TEAM, __FILE__": try breakaway=(%.1f %.1f)",
  //        target_point.x, target_point.y);
      breakaway = true;
    }
  }

  const double dash_power = (
      breakaway ?
          ServerParam::i().maxPower() : getDashPower(agent, target_point) );

  if (dash_power < 1.0)
  {
    agent->debugClient().addMessage("AttackGoRecover");
    agent->debugClient().setTarget(target_point);
    AngleDeg face_angle = wm.ball().angleFromSelf() + 90.0;
    if (face_angle.abs() > 90.0)
      face_angle += 180.0;
    Body_TurnToAngle(face_angle).execute(agent);
    agent->setNeckAction(new Neck_TurnToBallOrScan());
    return true;
  }

  ///////////////////////////////////////////////////

  agent->debugClient().addMessage("AttackGo%.0f", dash_power);

  double dist_thr = std::fabs(wm.ball().pos().x - wm.self().pos().x) * 0.2
      + 0.25;
  //double dist_thr = wm.ball().pos().dist( target_point ) * 0.1 + 0.5;
  if (dist_thr < 1.0)
    dist_thr = 1.0;
  if (target_point.x > wm.self().pos().x - 0.5
      && wm.self().pos().x < wm.offsideLineX()
      && std::fabs(target_point.x - wm.self().pos().x) > 1.0)
  {
    dist_thr = std::min(1.0, wm.ball().pos().dist(target_point) * 0.1 + 0.5);
  }

 // agent->debugClient().setTarget(target_point);
//  agent->debugClient().addCircle(target_point, dist_thr);
 // dlog.addText(Logger::TEAM,
//      __FILE__": execute. go to (%.2f, %.2f) dash_power=%.2f", target_point.x,
//      target_point.y, dash_power);

  if (Body_GoToPoint(target_point, dist_thr, dash_power, 100, // cycle
      false, // back
      true, // stamina save
      25.0 // angle threshold
      ).execute(agent))
  {
    if (intentional)
    {
 //     agent->debugClient().addMessage("BreakAway");

  //    agent->debugClient().addMessage("Sayh");
      agent->addSayMessage(new PassRequestMessage(target_point));

 //     dlog.addText(Logger::TEAM, __FILE__": intention breakaway");
      agent->setArmAction(new Arm_PointToPoint(target_point));
    }
    #ifdef DEBUG2014
    std::cerr << wm.time().cycle() << __FILE__ << wm.self().unum() << ": Body_GoToPoint.\n";
    #endif // DEBUG2014

  }
  else
  {
    AngleDeg body_angle(0.0);
    Body_TurnToAngle(body_angle).execute(agent);
  }

  if (agent->world().ball().posCount() <= 1)
  {
    agent->setNeckAction(new Neck_ScanField());
  }
  else
  {
    agent->setNeckAction(new Neck_TurnToBallOrScan());
  }

  return true;
}

/*-------------------------------------------------------------------*/
/*!
    ht: 20140905
    from roleoffensivehalfmove::doGoToCrossPoint( rcsc::PlayerAgent * agent, const rcsc::Vector2D & home_pos )
    need to change
*/
void
Bhv_AttackersMove::doGoToCrossPoint( rcsc::PlayerAgent * agent)
{
    rcsc::dlog.addText( rcsc::Logger::ROLE,
                        __FILE__": doGoToCrossPoint" );

    const rcsc::WorldModel & wm = agent->world();
    //------------------------------------------------------
    // tackle
    if ( Bhv_BasicTackle( 0.75, 90.0 ).execute( agent ) )
    {
        return;
    }

    //----------------------------------------------
    // intercept check
    int self_min = wm.interceptTable()->selfReachCycle();
    int mate_min = wm.interceptTable()->teammateReachCycle();
    int opp_min = wm.interceptTable()->opponentReachCycle();

    if ( self_min < mate_min
         || ( mate_min != 0 // ! wm.existKickableTeammate()  //try mate_min > 1
              && self_min <= 6
              && wm.ball().pos().dist( M_home_pos ) < 10.0 )
         //|| wm.interceptTable()->isSelfFastestPlayer()
         )
    {
        rcsc::dlog.addText( rcsc::Logger::ROLE,
                            __FILE__": doGoToCross. get ball" );
        agent->debugClient().addMessage( "CrossGetBall" );

        rcsc::Body_Intercept2009().execute( agent );

        if ( self_min == 3 && opp_min >= 3 )
        {
            agent->setViewAction( new rcsc::View_Normal() );
        }

        if ( wm.self().pos().x > 30.0 )
        {
            if ( ! doCheckCrossPoint( agent ) )
            {
                agent->setNeckAction( new rcsc::Neck_TurnToGoalieOrScan() );
            }
        }
        else
        {
            agent->setNeckAction( new rcsc::Neck_TurnToBallOrScan() );
        }
        return;
    }

    //----------------------------------------------
    // ball owner check
    if ( ! wm.interceptTable()->isOurTeamBallPossessor() )
    {
        const rcsc::PlayerObject * opp = wm.getOpponentNearestToBall( 3 );

        if ( opp
             && opp->distFromSelf() < 2.0 ) //3.0?
        {
            rcsc::dlog.addText( rcsc::Logger::ROLE,
                                __FILE__": doGoToCross. opp has ball" );
            agent->debugClient().addMessage( "CrossOppOwn(2)" );
            Bhv_BasicMove().execute( agent );
            return;
        }
    }

    //----------------------------------------------
    // set target

    rcsc::Vector2D target_point = M_home_pos;
    rcsc::Vector2D trap_pos = ( mate_min < 100
                                ? wm.ball().inertiaPoint( mate_min )
                                : wm.ball().pos() );

    if ( mate_min <= opp_min
         && mate_min < 3
         && target_point.x < 38.0
         && wm.self().pos().x < wm.offsideLineX() - 1.0
         //&& target_point.x < wm.self().pos().x
         //&& std::fabs( target_point.x - wm.self().pos().x ) < 20.0
         && std::fabs( target_point.y - wm.self().pos().y ) < 5.0
         && std::fabs( wm.self().pos().y - trap_pos.y ) < 13.0
         )
    {
        target_point.y = wm.self().pos().y * 0.9 + M_home_pos.y * 0.1;
        rcsc::dlog.addText( rcsc::Logger::ROLE,
                            __FILE__": doGoToCross. chance keep current." );
        agent->debugClient().addMessage( "CrossCurPos" );
    }

    // consider near opponent
    if ( target_point.x > 36.0 )
    {
        double opp_dist = 200.0;
        const rcsc::PlayerObject * opp = wm.getOpponentNearestTo( target_point,
                                                                  10,
                                                                  &opp_dist );
        if ( opp && opp_dist < 2.0 )
        {
            rcsc::Vector2D tmp_target = target_point;
            for ( int i = 0; i < 3; ++i )
            {
                tmp_target.x -= 1.0;

                double d = 0.0;
                opp = wm.getOpponentNearestTo( tmp_target, 10, &d );
                if ( ! opp )
                {
                    opp_dist = 0.0;
                    target_point = tmp_target;
                    break;
                }
                if ( opp
                     && opp_dist < d )
                {
                    opp_dist = d;
                    target_point = tmp_target;
                }
            }
            rcsc::dlog.addText( rcsc::Logger::ROLE,
                                __FILE__": doGoToCross. avoid(%.2f, %.2f)->(%.2f, %.2f)",
                                M_home_pos.x, M_home_pos.y,
                                target_point.x, target_point.y );
            agent->debugClient().addMessage( "Avoid" );
        }
    }

    if ( target_point.dist( trap_pos ) < 6.0 )
    {
        rcsc::Circle2D target_circle( trap_pos, 6.0 );
        rcsc::Line2D target_line( target_point, rcsc::AngleDeg( 90.0 ) );
        rcsc::Vector2D sol_pos1, sol_pos2;
        int n_sol = target_circle.intersection( target_line, &sol_pos1, &sol_pos2 );

        if ( n_sol == 1 ) target_point = sol_pos1;
        if ( n_sol == 2 )
        {
            target_point = ( wm.self().pos().dist2( sol_pos1 ) < wm.self().pos().dist2( sol_pos2 )
                             ? sol_pos1
                             : sol_pos2 );

        }

        rcsc::dlog.addText( rcsc::Logger::ROLE,
                            __FILE__": doGoToCross. adjust ot avoid the ball owner." );
        agent->debugClient().addMessage( "Adjust" );
    }

    //----------------------------------------------
    // set dash power
    // check X buffer & stamina
    static bool s_recover_mode = false;
    if ( wm.self().pos().x > 35.0
         && wm.self().stamina()
         < rcsc::ServerParam::i().recoverDecThrValue() + 500.0 )
    {
        s_recover_mode = true;
        rcsc::dlog.addText( rcsc::Logger::ROLE,
                            __FILE__": doGoToCross. recover on" );
    }

    if ( wm.self().stamina() > rcsc::ServerParam::i().staminaMax() * 0.5 )
    {
        s_recover_mode = false;
        rcsc::dlog.addText( rcsc::Logger::ROLE,
                            __FILE__": doGoToCross. recover off" );
    }

    double dash_power = rcsc::ServerParam::i().maxPower();
    if ( s_recover_mode )
    {
        const double my_inc
            = wm.self().playerType().staminaIncMax()
            * wm.self().recovery();
        dash_power = std::max( 1.0, my_inc - 25.0 );
        //dash_power = wm.self().playerType().staminaIncMax() * 0.6;
    }
    else if ( wm.ball().pos().x > wm.self().pos().x )
    {
        if ( wm.existKickableTeammate()
             && wm.ball().distFromSelf() < 10.0
             && std::fabs( wm.self().pos().x - wm.ball().pos().x ) < 5.0
             && wm.self().pos().x > 30.0
             && wm.ball().pos().x > 35.0 )
        {
            dash_power *= 0.5;
        }
    }
    else if ( wm.self().pos().dist( target_point ) < 3.0 )
    {
        const double my_inc
            = wm.self().playerType().staminaIncMax()
            * wm.self().recovery();
        dash_power = std::min( rcsc::ServerParam::i().maxPower(),
                               my_inc + 10.0 );
        //dash_power = rcsc::ServerParam::i().maxPower() * 0.8;
    }
    else if ( mate_min <= 1
              && wm.ball().pos().x > 33.0
              && wm.ball().pos().absY() < 7.0
              && wm.ball().pos().x < wm.self().pos().x
              && wm.self().pos().x < wm.offsideLineX()
              && wm.self().pos().absY() < 9.0
              && std::fabs( wm.ball().pos().y - wm.self().pos().y ) < 3.5
              && std::fabs( target_point.y - wm.self().pos().y ) > 5.0 )
    {
        dash_power = wm.self().playerType()
            .getDashPowerToKeepSpeed( 0.3, wm.self().effort() );
        dash_power = std::min( rcsc::ServerParam::i().maxPower() * 0.75,
                               dash_power );
        rcsc::dlog.addText( rcsc::Logger::ROLE,
                            __FILE__": doGoToCross. slow for cross. power=%.1f",
                            dash_power );
    }

#if 1


    bool intentional = true;    //false;

    const Vector2D mate_trap_pos = wm.ball().inertiaPoint(mate_min);

    const double max_x = std::max(wm.offsideLineX(), mate_trap_pos.x);

    if (wm.existKickableTeammate() || mate_min <= 5 || mate_min <= opp_min + 1)
    {
    }
    else if (M_forward_player
    //&& wm.audioMemory().dribbleTime() != wm.time()
        && mate_min <= 2 && opp_min >= 3
        && mate_trap_pos.x > wm.offsideLineX() - 10.0
        && wm.self().stamina() > ServerParam::i().staminaMax() * 0.8
        && 5.0 < wm.self().pos().x && wm.self().pos().x < 27.0
        && wm.self().pos().x > max_x - 7.0 && wm.self().pos().x < max_x - 1.0
        && wm.self().pos().x < mate_trap_pos.x + 10.0
        && (std::fabs(mate_trap_pos.y - wm.self().pos().y) < 8.0
            || (mate_trap_pos - wm.self().pos()).th().abs() < 110.0
        //|| ( mate_trap_pos.x > wm.offsideLineX() - 8.0
        //     && std::fabs( mate_trap_pos.y - wm.self().pos().y ) < 20.0 )
        ) && wm.self().pos().dist(mate_trap_pos) < 20.0
        && std::fabs(M_home_pos.y - wm.self().pos().y) < 15.0)
    {
      double x_diff = max_x - wm.self().pos().x;
      int dash_step = wm.self().playerType().cyclesToReachDistance(x_diff);
      if (mate_min < dash_step - 1)
      {
        target_point.x = std::min(wm.self().pos().x + 20.0, 50.0);
        target_point.y = wm.self().pos().y * 0.8 + mate_trap_pos.y * 0.2;
        if (target_point.absY() > 8.0
            && target_point.absY() > M_home_pos.absY())
        {
//          target_point.y = M_home_pos.y;
        }
        intentional = true;
      }
    }
#endif // 1

    //----------------------------------------------
    // positioning to make the cross course!!

    double dist_thr = wm.ball().distFromSelf() * 0.1;
    if ( dist_thr < 0.5 ) dist_thr = 0.5;

    agent->debugClient().addMessage( "GoToCross%.0f", dash_power );
    agent->debugClient().setTarget( target_point );
    agent->debugClient().addCircle( target_point, dist_thr );

    rcsc::dlog.addText( rcsc::Logger::ROLE,
                        __FILE__": doGoToCross. to (%.2f, %.2f)",
                        target_point.x, target_point.y );

    if ( wm.self().pos().x > target_point.x + dist_thr
         && std::fabs( wm.self().pos().x - target_point.x ) < 3.0
         && wm.self().body().abs() < 10.0 )
    {
        agent->debugClient().addMessage( "Back" );
        double back_dash_power
            = wm.self().getSafetyDashPower( -dash_power );
        rcsc::dlog.addText( rcsc::Logger::ROLE,
                            __FILE__": doShootAreaMove. Back Move" );
        agent->doDash( back_dash_power );
    }
    else
    {
        if ( ! rcsc::Body_GoToPoint( target_point, dist_thr, dash_power,
                                     5, // cycle
                                     false, // no back dash
                                     true, // save recovery
                                     30.0 // dir thr
                                     ).execute( agent ) )
        {
            rcsc::Body_TurnToAngle( 0.0 ).execute( agent );
        }
        else
        {
            if (intentional)
            {
         //     agent->debugClient().addMessage("BreakAway");

          //    agent->debugClient().addMessage("Sayh");
              agent->addSayMessage(new PassRequestMessage(target_point));

         //     dlog.addText(Logger::TEAM, __FILE__": intention breakaway");
              agent->setArmAction(new Arm_PointToPoint(target_point));
            }
        }
    }



    if ( wm.self().pos().x > 30.0 )
    {
        agent->setNeckAction( new rcsc::Neck_TurnToGoalieOrScan() );
    }
    else
    {
        agent->setNeckAction( new rcsc::Neck_TurnToLowConfTeammate() );
    }
}
// 2017.7       进攻的时候把7，8号调到禁区附近

bool
 Bhv_AttackersMove::gotomiddle( rcsc::PlayerAgent * agent )
{

const rcsc::WorldModel & wm = agent->world();
 Vector2D target_point = M_home_pos;
int term = (agent->world().time().cycle()/3) % 3;
bool flag=1;

if(wm.existKickableTeammate())
{
if(wm.self().unum()==7&&wm.ball().pos().x>10.0)
{
while(flag)
{
if(term==0)
{
target_point.y=8;
target_point.x= wm.self().pos().x + 3 ;
}
if(term==1)
{
target_point.y=7;
target_point.x= wm.self().pos().x + 2.5 ;
}

if(term==2)
{
target_point.y=6;
target_point.x= wm.self().pos().x + 2.7 ;
}
double dash_power1 = getDashPower(agent, target_point)*2;
        double dist_thr1 = std::fabs(wm.ball().pos().x - wm.self().pos().x) * 0.2 + 0.25;
         rcsc::Body_GoToPoint( target_point, dist_thr1, dash_power1,
                                     5, // cycle
                                     false, // no back dash
                                     true, // save recovery
                                     30.0 // dir thr
                                     ).execute( agent );
if(target_point.x>45.0)
flag=0;
}

}
if(wm.self().unum()==8&&wm.ball().pos().x>10.0)
{
flag=1;
while(flag)
{
if(term==0)
{
target_point.y=-8;
target_point.x= wm.self().pos().x + 2.6 ;
}
if(term==1)
{
target_point.y=-7;
target_point.x= wm.self().pos().x + 2.8 ;
}

if(term==2)
{
target_point.y=-6;
target_point.x= wm.self().pos().x + 3 ;
}
double dash_power1 = getDashPower(agent, target_point)*2;
        double dist_thr1 = std::fabs(wm.ball().pos().x - wm.self().pos().x) * 0.2 + 0.25;
         rcsc::Body_GoToPoint( target_point, dist_thr1, dash_power1,
                                     5, // cycle
                                     false, // no back dash
                                     true, // save recovery
                                     30.0 // dir thr
                                     ).execute( agent );
if(target_point.x>45.0)
flag=0;
}
}
return true;
}

return false;
}
//2017.7   配合7，8号的传接球函数
void
 Bhv_AttackersMove::finalshoot( rcsc::PlayerAgent * agent )
{
const rcsc::WorldModel & wm = agent->world();
    Vector2D target_point = wm.self().pos();
    double ball_speed = wm.self().kickRate() * ServerParam::i().maxPower();
      int mate_min = wm.interceptTable()->teammateReachCycle();
      Vector2D receive_pos = wm.ball().inertiaPoint(mate_min);
      int countOfOpps_mate=0;
      bool existoppblockball=0;
      const PlayerPtrCont::const_iterator t_end = wm.opponentsFromSelf().end();
      if ( Bhv_Shoot2008().execute(agent) )
        return ;
    for ( PlayerPtrCont::const_iterator t = wm.opponentsFromSelf().begin();
          t != t_end;
          ++t )
        {

            if((*t)->pos().dist(receive_pos)+ (*t)->pos().dist(target_point)<=receive_pos.dist( target_point)+2)
            existoppblockball=1;
                      if ( (*t)->pos().dist( receive_pos)<=2)
            {
                countOfOpps_mate++;
            }
        }
    if(gotomiddle(  agent )&& doKickToShooter(agent)&&!existoppblockball&& countOfOpps_mate<1)
    {
        Body_KickOneStep( receive_pos , ball_speed ).execute( agent );
        //agent->setNeckAction( new Neck_ScanField() );

    }
}
/*-------------------------------------------------------------------*/
/*!
    ht: 20140905
    from roleoffensivehalfmove::doCheckCrossPoint( rcsc::PlayerAgent * agent )
    for doGoToCrossPoint()
*/
bool
Bhv_AttackersMove::doCheckCrossPoint( rcsc::PlayerAgent * agent )
{
    const rcsc::WorldModel & wm = agent->world();

    if ( wm.self().pos().x < 35.0 )
    {
        return false;
    }

    const rcsc::PlayerObject * opp_goalie = wm.getOpponentGoalie();
    if ( opp_goalie && opp_goalie->posCount() > 2 )
    {
        rcsc::dlog.addText( rcsc::Logger::ROLE,
                            __FILE__": doCheckCrossTarget  goalie should be checked" );
        return false;
    }

    rcsc::Vector2D opposite_pole( 46.0, 7.0 );
    if ( wm.self().pos().y > 0.0 ) opposite_pole.y *= -1.0;

    rcsc::AngleDeg opposite_pole_angle = ( opposite_pole - wm.self().pos() ).th();


    if ( wm.dirCount( opposite_pole_angle ) <= 1 )
    {
        rcsc::dlog.addText( rcsc::Logger::ROLE,
                            __FILE__": doCheckCrossTarget enough accuracy to angle %.1f",
                            opposite_pole_angle.degree() );
        return false;
    }

    rcsc::AngleDeg angle_diff
        = agent->effector().queuedNextAngleFromBody( opposite_pole );
    if ( angle_diff.abs() > 100.0 )
    {
        rcsc::dlog.addText( rcsc::Logger::ROLE,
                            __FILE__": doCheckCrossPoint. want to face opposite pole,"
                            " but over view range. angle_diff=%.1f",
                            angle_diff.degree() );
        return false;
    }


    agent->setNeckAction( new rcsc::Neck_TurnToPoint( opposite_pole ) );
    agent->debugClient().addMessage( "NeckToOpposite" );
    rcsc::dlog.addText( rcsc::Logger::ROLE,
                        __FILE__": doCheckCrossPoint Neck to oppsite pole" );
    return true;
}

bool
Bhv_AttackersMove::AttackChance(PlayerAgent *agent)
{
    const WorldModel & wm = agent->world();
    if((wm.self().pos().x > 47&&wm.self().pos().absY() < 13)
    ||(wm.self().pos().x > 41&&wm.self().pos().absY() < 8.5))
    {
    }
    else
        return false;
    const PlayerPtrCont opps = wm.opponentsFromSelf();
    Vector2D goal_c = ServerParam::i().theirTeamGoalPos();
    Vector2D upGoal;
        upGoal.assign(52.5,-6.5);
    Vector2D downGoal;
        downGoal.assign(52.5,6.5);
    Vector2D target_point = wm.self().pos();
    double NearstOppsToLine1 = 20,NearstOppsToLine2 = 20;
    double oppsToLine;
    Line2D line1(target_point,upGoal) , line2(target_point,downGoal);

    const PlayerPtrCont::const_iterator t_end = wm.opponentsFromSelf().end();
    for ( PlayerPtrCont::const_iterator t = wm.opponentsFromSelf().begin();
      t != t_end;
      ++t )
      {
            oppsToLine = line1.dist((*t)->pos());
            if( oppsToLine< NearstOppsToLine1)
                NearstOppsToLine1 = oppsToLine;
      }
      for ( PlayerPtrCont::const_iterator t = wm.opponentsFromSelf().begin();
      t != t_end;
      ++t )
      {
            oppsToLine = line2.dist((*t)->pos());
            if( oppsToLine< NearstOppsToLine2)
                NearstOppsToLine2 = oppsToLine;
      }
      if ((NearstOppsToLine1 < 2) && (NearstOppsToLine2 < 2)) return false;
    return true;
}

bool
Bhv_AttackersMove::doassess(PlayerAgent * agent)
{

    const WorldModel & wm = agent->world();
    const PlayerPtrCont opps = wm.opponentsFromSelf();
    Vector2D target_point = wm.self().pos();
    Vector2D goal_c = ServerParam::i().theirTeamGoalPos();
    int countOfOpps = 0;
    double degreeOfdiffclt=0.0;

    const PlayerPtrCont::const_iterator t_end = wm.opponentsFromSelf().end();
    for ( PlayerPtrCont::const_iterator t = wm.opponentsFromSelf().begin();
          t != t_end;
          ++t )
        {
            if ( (*t)->pos().dist(target_point)<3 && (*t)->pos().x > target_point.x)
            {
                countOfOpps++;
            }
        }

        if(countOfOpps >= 2
        || target_point.dist(goal_c)>15
        ||!AttackChance( agent ))
            return false;
        return true;
/*        &&target_point.dist(goal_c)<5
        &&target_point.absY()<7.0)
            return true;
        else
        return false;*/
}

bool
Bhv_AttackersMove::doCoordinate( PlayerAgent * agent)
{
    const WorldModel & wm = agent->world();
    double ball_speed = wm.self().kickRate() * ServerParam::i().maxPower();
    rcsc::Vector2D goal_c = ServerParam::i().theirTeamGoalPos();
    if ( doassess(agent) )
    {
        Body_KickOneStep( goal_c, ball_speed ).execute( agent );
        //agent->setNeckAction( new Neck_ScanField() );
        return true;
    }
    else if( doKickToShooter(agent) )
        return true;
    return false;



}

/*-------------------------------------------------------------------*/
/*!

 */
double
Bhv_AttackersMove::getDashPower(const PlayerAgent * agent,
    const Vector2D & target_point)
{
  const WorldModel & wm = agent->world();
  int mate_min = wm.interceptTable()->teammateReachCycle();
  int opp_min = wm.interceptTable()->opponentReachCycle();

  if (target_point.x > wm.self().pos().x
      && wm.self().stamina() > ServerParam::i().staminaMax() * 0.7
      && mate_min <= 8)
  {
    Vector2D receive_pos = wm.ball().inertiaPoint(mate_min);
    if (std::fabs(receive_pos.y - target_point.y) < 15.0)
    {
 //     dlog.addText(Logger::TEAM, __FILE__": getDashPower. chance. fast move");
      return ServerParam::i().maxPower();
    }
  }

  if (wm.self().pos().x > wm.offsideLineX()
      && (wm.existKickableTeammate() || mate_min <= opp_min + 2))
  {
    Vector2D receive_pos = wm.ball().inertiaPoint(mate_min);
    if (target_point.x < receive_pos.x + 30.0
        && wm.self().pos().dist(receive_pos) < 30.0
        && wm.self().pos().dist(target_point) < 20.0)
    {
//      dlog.addText(Logger::TEAM, __FILE__": getDashPower. offside max power");

      return ServerParam::i().maxPower();
    }
  }

  //------------------------------------------------------
  // decide dash power
  static bool s_recover_mode = false;
  if (wm.self().stamina() < ServerParam::i().staminaMax() * 0.4)
  {
 //   dlog.addText(Logger::TEAM,
 //       __FILE__": getDashPower. change to recover mode.");
    s_recover_mode = true;
  }
  else if (wm.self().stamina() > ServerParam::i().staminaMax() * 0.7)
  {
//    dlog.addText(Logger::TEAM,
//        __FILE__": getDashPower. came back from recover mode");
    s_recover_mode = false;
  }

  const double my_inc = wm.self().playerType().staminaIncMax()
      * wm.self().recovery();

  // dash power
  if (s_recover_mode)
  {
    // Magic Number.
    // recommended one cycle's stamina recover value
 //   dlog.addText(Logger::TEAM, __FILE__": getDashPower. recover mode");
    return std::max(0.0, my_inc - 30.0);
  }

  if (!wm.opponentsFromSelf().empty()
      && wm.opponentsFromSelf().front()->distFromSelf() < 1.0)
  {
    // opponent is very close
    // full power
    dlog.addText(Logger::TEAM,
        __FILE__": getDashPower. exist near opponent. full power");
    return ServerParam::i().maxPower();
  }

  if (wm.ball().pos().x < wm.self().pos().x
      && wm.self().pos().x < wm.offsideLineX())
  {
    // ball is back
    // not offside
 //   dlog.addText(Logger::TEAM,
//        __FILE__": getDashPower. ball is back and not offside.");
    if (wm.self().stamina() < ServerParam::i().staminaMax() * 0.8)
    {
      return std::min(std::max(5.0, my_inc - 30.0), ServerParam::i().maxPower());
    }
    else
    {
      return std::min(my_inc * 1.1, ServerParam::i().maxPower());
    }
  }

  if (wm.ball().pos().x > wm.self().pos().x + 3.0)
  {
    // ball is front
    if (opp_min <= mate_min - 3)
    {
      if (wm.self().stamina() < ServerParam::i().staminaMax() * 0.6)
      {
  //      dlog.addText(Logger::TEAM,
  //          __FILE__": getDashPower. ball is front. recover");
        return std::min(std::max(0.1, my_inc - 30.0),
            ServerParam::i().maxPower());
      }
      else if (wm.self().stamina() < ServerParam::i().staminaMax() * 0.8)
      {
 //       dlog.addText(Logger::TEAM,
 //           __FILE__": getDashPower. ball is front. keep");
        return std::min(my_inc, ServerParam::i().maxPower());
      }
      else
      {
 //       dlog.addText(Logger::TEAM,
//            __FILE__": getDashPower. ball is front. max");
        return ServerParam::i().maxPower();
      }
    }
    else
    {
 //     dlog.addText(Logger::TEAM,
 //         __FILE__": getDashPower. ball is front full powerr");
      return ServerParam::i().maxPower();
    }
  }

 // dlog.addText(Logger::TEAM, __FILE__": getDashPower. normal mode.");
  if (target_point.x > wm.self().pos().x + 2.0
      && wm.self().stamina() > ServerParam::i().staminaMax() * 0.6)
  {
    return ServerParam::i().maxPower();
  }
  else if (wm.self().stamina() < ServerParam::i().staminaMax() * 0.8)
  {
    return std::min(my_inc * 0.9, ServerParam::i().maxPower());
  }
  else
  {
    return std::min(my_inc * 1.5, ServerParam::i().maxPower());
  }
}

bool
Bhv_AttackersMove::Body_ForestallBlock(PlayerAgent * agent)
{
  int S_count_no_move = 0 ;
  const rcsc::WorldModel & wm = agent->world();
  rcsc:Vector2D home_pos = Strategy::i().getPosition(wm.self().unum());
  rcsc::Vector2D block_point = home_pos;

  if (!block_point.isValid())
  {
    return false;
  }

  double dist_thr = 1.0;

 // agent->debugClient().addMessage("forestall");
 // agent->debugClient().setTarget(block_point);
 // agent->debugClient().addCircle(block_point, dist_thr);

  double dash_power = wm.self().getSafetyDashPower(
      rcsc::ServerParam::i().maxPower());

  if (rcsc::Body_GoToPoint(block_point, dist_thr, dash_power, 5, // cycle
      false, // no back dash
      true, // save recovery
      40.0 // dir_thr
      ).execute(agent))
  {
    S_count_no_move = 0;
    return true;
  }

#if 1
  ++S_count_no_move;

  if (S_count_no_move >= 10)
  {
 //   rcsc::dlog.addText(rcsc::Logger::TEAM,
 //       __FILE__": forestall. long no move. attack to the ball");
 //   agent->debugClient().addMessage("ForestallAttack");
    Body_Intercept2009(true).execute(agent);
    return true;
  }
#endif

  rcsc::AngleDeg body_angle = wm.ball().angleFromSelf() + 90.0;
  if (body_angle.abs() < 90.0)
  {
    body_angle += 180.0;
  }

  rcsc::Body_TurnToAngle(body_angle).execute(agent);

  return true;
}

bool
Bhv_AttackersMove::doKickToShooter( PlayerAgent * agent )
{
    const WorldModel & wm = agent->world();


    const Vector2D goal( ServerParam::i().pitchHalfLength(),
                         wm.self().pos().y * 0.8 );

    double min_dist = 100000.0;
    const PlayerObject * receiver = static_cast< const PlayerObject * >( 0 );

    const PlayerPtrCont::const_iterator t_end = wm.teammatesFromBall().end();
    for ( PlayerPtrCont::const_iterator t = wm.teammatesFromBall().begin();
          t != t_end;
          ++t )
    {
        if ( (*t)->posCount() > 5 ) continue;
        //if ( (*t)->distFromBall() < 1.5 ) continue;
        if ( (*t)->distFromBall() > 20.0 ) continue;
        if ( (*t)->pos().x > wm.offsideLineX() ) continue;
    //    if ( (*t)->pos().x < wm.ball().pos().x - 3.0 ) continue;
        //if ( (*t)->pos().absY() > ServerParam::i().goalHalfWidth() * 0.5 ) continue;

        double goal_dist = (*t)->pos().dist( goal );
        if ( goal_dist > 16.0 )
        {
            continue;
        }

        double dist = goal_dist * 0.4 + (*t)->distFromBall() * 0.6;

        if ( dist < min_dist )
        {
            min_dist = dist;
            receiver = (*t);
        }
    }

    if ( ! receiver )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": (doKicToShooter) no shooter" );
        return false;
    }

    const double max_ball_speed = wm.self().kickRate() * ServerParam::i().maxPower();

    Vector2D target_point = receiver->pos() + receiver->vel();
    target_point.x += 0.6;

    double target_dist = wm.ball().pos().dist( target_point );

    int ball_reach_step
        = static_cast< int >( std::ceil( calc_length_geom_series( max_ball_speed,
                                                                  target_dist,
                                                                  ServerParam::i().ballDecay() ) ) );
    double ball_speed = calc_first_term_geom_series( target_dist,
                                                     ServerParam::i().ballDecay(),
                                                     ball_reach_step );

    ball_speed = std::min( ball_speed, max_ball_speed );

    agent->debugClient().addMessage( "IndKick:KickToShooter%.3f", ball_speed );
    agent->debugClient().setTarget( target_point );
    dlog.addText( Logger::TEAM,
                  __FILE__":  pass to nearest teammate (%.1f %.1f) ball_speed=%.2f reach_step=%d",
                  target_point.x, target_point.y,
                  ball_speed, ball_reach_step );

    Body_KickOneStep( target_point, ball_speed ).execute( agent );
    agent->setNeckAction( new Neck_ScanField() );

    return true;

}














