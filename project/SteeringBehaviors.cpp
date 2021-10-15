//Precompiled Header [ALWAYS ON TOP IN CPP]
#include "stdafx.h"

//Includes
#include "SteeringBehaviors.h"
#include "IExamInterface.h"
//SEEK
//****
SteeringPlugin_Output Seek::CalculateSteering(float deltaT, IExamInterface* pInterface)
{
	SteeringPlugin_Output steering{};
	AgentInfo agent{pInterface->Agent_GetInfo()};
	steering.LinearVelocity = pInterface->NavMesh_GetClosestPathPoint(m_Target.Position) - agent.Position; //Desired v
	steering.LinearVelocity.Normalize(); //normalize to control speed
	steering.LinearVelocity *= agent.MaxLinearSpeed; //rescale to max speed


	return steering; //return steering
}
bool Seek::HasReachedDestination(const IExamInterface* pInterface) const
{
	AgentInfo agent{ pInterface->Agent_GetInfo() };
	float fromTargetSqr{ Elite::Vector2(m_Target.Position - agent.Position).SqrtMagnitude() };
	return fromTargetSqr <= (agent.AgentSize * agent.AgentSize);
}

//WANDER (base> SEEK)
//******
SteeringPlugin_Output Wander::CalculateSteering(float deltaT, IExamInterface* pInterface)
{
	SteeringPlugin_Output steering{};
	AgentInfo agent{ pInterface->Agent_GetInfo() };
	Vector2 dir{cosf(agent.Orientation - static_cast<float>(M_PI) /2), sinf(agent.Orientation- static_cast<float>(M_PI) /2)};
	dir = dir.GetNormalized();

	Elite::Vector2 circle{ agent.Position + dir * m_WanderOffset }; //circle center location
	m_WanderAngle = randomFloat(m_WanderAngle - m_WanderMaxAngle / 2, m_WanderAngle + m_WanderMaxAngle / 2);
	//std::cout << m_WanderAngle << std::endl;//new wander angle is inbetween boundaries of prev angle and max change
	Elite::Vector2 pointOnCircle{circle.x + m_WanderRadius * cosf(ToRadians(m_WanderAngle)), circle.y + m_WanderRadius * sinf(ToRadians(m_WanderAngle))}; //Calculate point on circle

	steering.LinearVelocity = pointOnCircle - agent.Position;
	steering.LinearVelocity.Normalize();
	steering.LinearVelocity *= agent.MaxLinearSpeed;

	
	return steering;
}

SteeringPlugin_Output Flee::CalculateSteering(float deltaT, IExamInterface* pInterface)
{
	SteeringPlugin_Output steering{};
	AgentInfo agent{ pInterface->Agent_GetInfo() };

	steering = Seek::CalculateSteering(deltaT, pInterface);
	steering.AngularVelocity *= -1;
	steering.LinearVelocity *= -1;

	return steering;
}


SteeringPlugin_Output Arrive::CalculateSteering(float deltaT, IExamInterface* pInterface)
{
	float radius{ 20 };
	SteeringPlugin_Output steering{};
	AgentInfo agent{ pInterface->Agent_GetInfo() };

	steering = Seek::CalculateSteering(deltaT, pInterface);
	float distanceAgentToTarget{ (m_Target.Position - agent.Position).Magnitude() };
	if (radius > distanceAgentToTarget)
	{
		steering.LinearVelocity = steering.LinearVelocity.GetNormalized() * agent.MaxLinearSpeed * (distanceAgentToTarget / radius);
	}
	else
		steering = Seek::CalculateSteering(deltaT, pInterface);

	return steering;
}


SteeringPlugin_Output Pursuit::CalculateSteering(float deltaT, IExamInterface* pInterface)
{
	SteeringPlugin_Output steering{};
	AgentInfo agent{ pInterface->Agent_GetInfo() };

	Elite::Vector2 toTarget{ m_Target.Position - agent.Position };
	float distanceToTarget{ toTarget.Magnitude() };
	//take future prediction of target pos as true target
	//Prediction is target pos + X amount of time in front of them (Add their velocity * distance/theirMaxSpeed)
	Elite::Vector2 toTargetFuture{(m_Target.Position + m_Target.LinearVelocity * (distanceToTarget/ agent.MaxLinearSpeed)) - agent.Position}; //distance/agentmaxspeed for a more realistic, responsive predict
	steering.LinearVelocity = toTargetFuture;
	steering.LinearVelocity.Normalize();
	steering.LinearVelocity *= agent.MaxLinearSpeed;

	return steering;
}

SteeringPlugin_Output Evade::CalculateSteering(float deltaT,  IExamInterface* pInterface) //AddRadius
{
	SteeringPlugin_Output steering{};
	AgentInfo agent{ pInterface->Agent_GetInfo() };
	steering.AutoOrient = true;
	//player dir (to make player face enemy)
	Elite::Vector2 toTarget{ m_Target.Position - agent.Position };
	float distanceToTarget{ toTarget.Magnitude() };
	if (distanceToTarget > m_EvadeRadius)
	{
		return steering;
	}
	toTarget.Normalize();
	//take future prediction of target pos as true target
	Elite::Vector2 toTargetFuture{ (m_Target.Position + Vector2{4,4} *(distanceToTarget / agent.MaxLinearSpeed)) - agent.Position }; //distance/agentmaxspeed for a more realistic, responsive predict
	steering.LinearVelocity = toTargetFuture;
	steering.LinearVelocity.Normalize();
	steering.LinearVelocity *= agent.MaxLinearSpeed; //We want to choose own speed

	//Make player face enemy
	Vector2 dir{ cosf(agent.Orientation), sinf(agent.Orientation) };
	float angle{Dot(toTarget, dir)/(toTarget.Magnitude()*dir.Magnitude())};
	steering.AngularVelocity = angle;
	std::cout << steering.AngularVelocity << std::endl;
	steering.LinearVelocity *= -1; //Same as pursuit (predicting a future target to go to but flee from that future target (pursuit mixed with flee)

	return steering;
}

SteeringPlugin_Output Dodge::CalculateSteering(float deltaT, IExamInterface* pInterface)
{
	SteeringPlugin_Output steering{};
	
	
	return steering;
}

SteeringPlugin_Output Face::CalculateSteering(float deltaT, IExamInterface* pInterface)
{

	SteeringPlugin_Output steering{ };
	Vector2 toTarget{ m_Target - pInterface->Agent_GetInfo().Position };
	Vector2 dir{ cosf(pInterface->Agent_GetInfo().Orientation), sinf(pInterface->Agent_GetInfo().Orientation) };
	float angle{ Dot(toTarget, dir) / (toTarget.Magnitude() * dir.Magnitude()) };
	steering.AutoOrient = false;
	steering.AngularVelocity = angle*10;
	
	pInterface->Draw_Segment(pInterface->Agent_GetInfo().Position, pInterface->Agent_GetInfo().Position + m_Target - pInterface->Agent_GetInfo().Position, Vector3{ 1,0,0 });
	std::cout << "FACE" << std::endl;
	return steering;
}

SteeringPlugin_Output FaceSeek::CalculateSteering(float deltaT, IExamInterface* pInterface)
{
	SteeringPlugin_Output steering{};
	AgentInfo agent{ pInterface->Agent_GetInfo() };
	steering.LinearVelocity = pInterface->NavMesh_GetClosestPathPoint(m_Target.Position) - agent.Position; //Desired v
	steering.LinearVelocity.Normalize(); //normalize to control speed
	steering.LinearVelocity *= agent.MaxLinearSpeed; //rescale to max speed


	steering.AutoOrient = false;
	steering.AngularVelocity = 10;

	return steering;
}
