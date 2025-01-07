
#include "PCH.h"
#include "WPhysState.h"

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			PhysState dynamics

	Author:			Magnus Högdahl

	Copyright:		Starbreeze Studios 2004

	Contents:		

	History:
		2004-12-19:	Created file

	Comments:


		Simulation step:

			for all active dynamics
				ApplyMediumAndGravitonalAccelleration();

			for all active dynamics
				AdvanceTemporaryPositions();

			for all active dynamics
				CollideUsingTemporaryPositionsAndStoreCollisionConstraints();

			for x number of times
			{
				for all collisions
					ApplyElasticCollisionImpulsesWithFriction();
				for all collisions
					RemoveNonPenetrating();

				Break if y amount of execution time has elapsed?
			}

			for all active dynamics
				AdvanceVelocities

			CreateContactGraph

			for(iGraphLevel = 0; iGraphLevel < nGraphLevels; iGraphLevel++)
			{
				for x number of times
				{
					for all collisions in graphlevel <= iGraphLevel
						ApplyNonElasticCollisionImpulses();
				}

				Break if y amount of execution time has elapsed?
			}

			CreateContactGraph?

			for(iGraphLevel = 0; iGraphLevel < nGraphLevels; iGraphLevel++)
			{
				for x number of times
				{
					for all collisions in graphlevel <= iGraphLevel
						ApplyNonElasticCollisionImpulses();
				}

				for all dynamics in iGraphLevel, 
					Tag as immobile / treat as infinite mass.

				Break if y amount of execution time has elapsed?
			}

			for all active dynamics
				AdvancePositions




	Activation/Deactivation?


\*____________________________________________________________________________________________*/


