/*=============================================================================*/
// Copyright 2017-2018 Elite Engine
// Authors: Matthieu Delaere, Thomas Goussaert
/*=============================================================================*/
// SteeringBehaviors.h: SteeringBehaviors interface and different implementations
/*=============================================================================*/
#ifndef ELITE_STEERINGBEHAVIORS
#define ELITE_STEERINGBEHAVIORS

//-----------------------------------------------------------------
// Includes & Forward Declarations
//-----------------------------------------------------------------
#include "Exam_HelperStructs.h"

#include "SteeringHelpers.h"
class IExamInterface;
using namespace Elite;

#pragma region **ISTEERINGBEHAVIOR** (BASE)
class ISteeringBehavior
{
public:
	ISteeringBehavior() = default;
	virtual ~ISteeringBehavior() = default;

	virtual SteeringPlugin_Output CalculateSteering(float deltaT,  IExamInterface* pInterface) = 0;

	//Seek Functions
	virtual void SetTarget(const TargetData& target) { m_Target = target; }

	template<class T, typename std::enable_if<std::is_base_of<ISteeringBehavior, T>::value>::type* = nullptr>
	T* As()
	{ return static_cast<T*>(this); }

protected:
	TargetData m_Target;
};
#pragma endregion

///////////////////////////////////////
//SEEK
//****
class Seek : public ISteeringBehavior
{
public:
	Seek() = default;
	virtual ~Seek() = default;

	//Seek Behaviour
	SteeringPlugin_Output CalculateSteering(float deltaT,  IExamInterface* pInterface) override;
	bool HasReachedDestination(const IExamInterface* pInterface) const;
};

//////////////////////////
//FLEE
//******
class Flee :public Seek
{
public:
	Flee() = default;
	virtual ~Flee() = default;

	SteeringPlugin_Output CalculateSteering(float deltaT,  IExamInterface* pInterface) override;
};

//////////////////////////
//ARRIVE
//******
class Arrive :public Seek
{
public:
	Arrive() :m_SlowRadius{ 15.f }, m_TargetRadius{10.f} {};
	virtual ~Arrive() = default;

	SteeringPlugin_Output CalculateSteering(float deltaT,  IExamInterface* pInterface) override;
	void SetSlowRadius(const float& radius) { m_SlowRadius = radius; }
	void SetTargetRadius(const float& radius) { m_TargetRadius = radius; }

private:
	float m_SlowRadius;
	float m_TargetRadius;

};
//////////////////////////


//FACE
//******
class Face final : public Seek
{
	public:
		Face() = default;

		virtual ~Face() = default;

		//Wander Behavior
		SteeringPlugin_Output CalculateSteering(float deltaT,  IExamInterface * pInterface) override;


	protected:
		Vector2 m_Target;

	private:
		void SetTarget(const TargetData& target) override { m_Target = target.Position; }//To hide the settarget function

};
//////////////////////////
//WANDER
//******
class Wander : public Seek
{
public:
	Wander() = default;

	virtual ~Wander() = default;

	//Wander Behavior
	SteeringPlugin_Output CalculateSteering(float deltaT,  IExamInterface* pInterface) override;

	void SetWanderOffset(const float& offset) { m_WanderOffset = offset; };
	void SetWanderRadius(const float& radius) { m_WanderRadius = radius; };
	void UpdateWanderAngle(const float& angle) { m_WanderAngle = angle; };
	void SetWanderMaxAngleChange(const float& maxAngle) { m_WanderMaxAngle = maxAngle; };


protected:
	float m_WanderOffset = 30.f; //distance from agent to wander circle center (farther == more subtle movements)
	float m_WanderRadius = 10.f; //radius of wander circle (bigger == more subtle movements)
	float m_WanderMaxAngle = 10.f; //max angle change per frame
	float m_WanderAngle = 70.f; //current angle to wander to
	
private:
	void SetTarget(const TargetData& target) override {}//To hide the settarget function

};

//////////////////////////
//PURSUIT
//******
class Pursuit :public Seek
{
public:
	Pursuit() = default;
	virtual ~Pursuit() = default;

	SteeringPlugin_Output CalculateSteering(float deltaT,  IExamInterface* pInterface) override;
};

//////////////////////////
//EVADE
//******
class Evade :public Seek
{
public:
	Evade() = default;
	virtual ~Evade() = default;

	SteeringPlugin_Output CalculateSteering(float deltaT,  IExamInterface* pInterface) override;
	void SetEvadeRadius(const float& evadeRadius) { m_EvadeRadius = evadeRadius; }
	//add radius
private:
	float m_EvadeRadius = 200.f;
};

//////////////////////////
//DODGE
//******
class Dodge :public Seek
{
public:
	Dodge() = default;
	virtual ~Dodge() = default;

	SteeringPlugin_Output CalculateSteering(float deltaT,  IExamInterface* pInterface) override;
	//add radius
private:
	float m_Range = 20.f;
	float m_Width = 20.f;

};

class FaceSeek final : public Seek
{

public:
	FaceSeek() = default;
	virtual ~FaceSeek() = default;
	SteeringPlugin_Output CalculateSteering(float deltaT, IExamInterface* pInterface) override;

};

#endif


