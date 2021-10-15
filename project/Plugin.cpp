#include "stdafx.h"
#include "Plugin.h"
#include "IExamInterface.h"
#include "EBehaviorTree.h"
#include "Behaviors.h"

Plugin::~Plugin()
{
	for (auto behaviorIt : m_pBehaviors)
	{
		SAFE_DELETE(behaviorIt.second);
	}
	SAFE_DELETE(m_pBT);

	
}

//Called only once, during initialization
void Plugin::Initialize(IBaseInterface* pInterface, PluginInfo& info)
{
	//Retrieving the interface
	//This interface gives you access to certain actions the AI_Framework can perform for you
	m_pInterface = static_cast<IExamInterface*>(pInterface);

	//Bit information about the plugin
	//Please fill this in!!
	info.BotName = "Hippy_Exterminator";
	info.Student_FirstName = "Jasper";
	info.Student_LastName = "Caerels";
	info.Student_Class = "2DAE02";

	InitGrid(100);
	InitBlackboard();
	InitBehavior();

}

//Called only once
void Plugin::DllInit()
{
	//Called when the plugin is loaded
}

//Called only once
void Plugin::DllShutdown()
{
	//Called wheb the plugin gets unloaded
}

//Called only once, during initialization
void Plugin::InitGameDebugParams(GameDebugParams& params)
{
	params.AutoFollowCam = true; //Automatically follow the AI? (Default = true)
	params.RenderUI = true; //Render the IMGUI Panel? (Default = true)
	params.SpawnEnemies = true; //Do you want to spawn enemies? (Default = true)
	params.EnemyCount = 20; //How many enemies? (Default = 20)
	params.GodMode = false; //GodMode > You can't die, can be usefull to inspect certain behaviours (Default = false)
	params.AutoGrabClosestItem = false; //A call to Item_Grab(...) returns the closest item that can be grabbed. (EntityInfo argument is ignored)
}

//Only Active in DEBUG Mode
//(=Use only for Debug Purposes)
void Plugin::Update(float dt)
{
	//Demo Event Code
	//In the end your AI should be able to walk around without external input
	if (m_pInterface->Input_IsMouseButtonUp(Elite::InputMouseButton::eLeft))
	{
		//Update target based on input
		Elite::MouseData mouseData = m_pInterface->Input_GetMouseData(Elite::InputType::eMouseButton, Elite::InputMouseButton::eLeft);
		const Elite::Vector2 pos = Elite::Vector2(static_cast<float>(mouseData.X), static_cast<float>(mouseData.Y));
		m_Target = m_pInterface->Debug_ConvertScreenToWorld(pos);
		m_pB->ChangeData("Target", m_Target);
	}
	else if (m_pInterface->Input_IsKeyboardKeyDown(Elite::eScancode_Space))
	{
		m_CanRun = true;
	}
	else if (m_pInterface->Input_IsKeyboardKeyDown(Elite::eScancode_Left))
	{
		m_AngSpeed -= Elite::ToRadians(10);
	}
	else if (m_pInterface->Input_IsKeyboardKeyDown(Elite::eScancode_Right))
	{
		m_AngSpeed += Elite::ToRadians(10);
	}
	else if (m_pInterface->Input_IsKeyboardKeyDown(Elite::eScancode_G))
	{
		m_GrabItem = true;
	}
	else if (m_pInterface->Input_IsKeyboardKeyDown(Elite::eScancode_U))
	{
		m_UseItem = true;
	}
	else if (m_pInterface->Input_IsKeyboardKeyDown(Elite::eScancode_R))
	{
		m_RemoveItem = true;
	}
	else if (m_pInterface->Input_IsKeyboardKeyUp(Elite::eScancode_Space))
	{
		m_CanRun = false;
	}
	DebugGrid();

}

//Update
//This function calculates the new SteeringOutput, called once per frame
SteeringPlugin_Output Plugin::UpdateSteering(float dt)
{
	HandleTimers(dt);
	
	//Use the Interface (IAssignmentInterface) to 'interface' with the AI_Framework
	m_pB->ChangeData("Agent", &m_pInterface->Agent_GetInfo());

	//auto nextTargetPos = m_Target; //To start you can use the mouse position as guidance

	//--------------AREA HANDLING--------------
	HandleHouses();
	HandleEntities();
	HandleItemManagement();
	//-----------------------------------------

	//BT
	m_pBT->Update(dt);
	//-----------------------------------------
	

	return HandleSteering(dt);
}

//This function should only be used for rendering debug elements
void Plugin::Render(float dt) const
{
	//This Render function should only contain calls to Interface->Draw_... functions
	m_pInterface->Draw_SolidCircle(m_Target, .7f, { 0,0 }, { 1, 0, 0 });
}

vector<HouseInfo> Plugin::GetHousesInFOV() const
{
	vector<HouseInfo> vHousesInFOV = {};

	HouseInfo hi = {};
	for (int i = 0;; ++i)
	{
		if (m_pInterface->Fov_GetHouseByIndex(i, hi))
		{
			vHousesInFOV.push_back(hi);
			continue;
		}

		break;
	}

	return vHousesInFOV;
}

vector<EntityInfo> Plugin::GetEntitiesInFOV() const
{
	vector<EntityInfo> vEntitiesInFOV = {};

	EntityInfo ei = {};
	for (int i = 0;; ++i)
	{
		if (m_pInterface->Fov_GetEntityByIndex(i, ei))
		{
			vEntitiesInFOV.push_back(ei);
			continue;
		}

		break;
	}

	return vEntitiesInFOV;
}



void Plugin::InitBlackboard()
{
	m_pB = new Blackboard();
	//Change behaviors and general data accessing
	m_pB->AddData("Behavior", std::string{"Wander"});
	m_pB->AddData("pInterface", m_pInterface);
	m_pB->AddData("Agent", &m_pInterface->Agent_GetInfo()); //is now obsolete, can delete/refactor some code
	m_pB->AddData("Enemies", std::vector<EnemyInfo*>());
	m_pB->AddData("Target", Vector2{0,0});
	m_pB->AddData("PurgeZones", std::vector<PurgeZoneInfo*>());
	m_pB->AddData("CloseToBorder", bool{});
	m_pB->AddData("Turning", bool{});
	m_pB->AddData("WanderTimer", float{});
	m_pB->AddData("Cooldown", &m_Cooldown);
	//Grid
	m_pB->AddData("Waypoints", &m_pWayPoints);


	//Houses
	m_pB->AddData("Houses", std::vector<HouseInfo*>());
	//Keep which house shouldnt be explored for forseeable future
	m_pB->AddData("ExploredHouses", std::vector<HouseInfo*>());
	//Save outside pos for when entering house (can maybe add more/different functionality to houseExploration)
	m_pB->AddData("OutsidePos", Vector2{});

	//Bools to keep specific states until switched
	m_pB->AddData("LeavingHouse", bool{});
	m_pB->AddData("GoingInside", bool{});

	//ItemManagement
	m_pB->AddData("Items", std::vector<std::pair<EntityInfo, ItemInfo*>>());
	m_pB->AddData("Inventory", &m_pInventory);
	m_pB->AddData("WantedType", eItemType{});
}

void Plugin::InitGrid(int cellSize)
{
	int worldWidth{ int(m_pInterface->World_GetInfo().Dimensions.x) };
	int worldHeight{ int(m_pInterface->World_GetInfo().Dimensions.y) };

	m_Cols = worldWidth / cellSize;
	m_Rows = worldHeight / cellSize;

	Vector2 currPoint{ m_pInterface->World_GetInfo().Center.x - worldWidth/2, m_pInterface->World_GetInfo().Center.y - worldHeight/2 };


	for (int i{}; i < m_Rows; i++)
	{
		for (int j{}; j < m_Cols; j++)
		{
			Vector2 waypoint{ currPoint.x + (cellSize * j + cellSize / 2.f), currPoint.y + (i *cellSize+cellSize/2.f) };
			m_pWayPoints[ i * m_Cols + j] = std::pair<bool, Vector2>{ false,waypoint };
		}
	}
}

void Plugin::HandleEntities()
{
	std::vector<std::pair<EntityInfo, ItemInfo*>> pItemsInSight{};
	std::vector<EnemyInfo*> pEnemiesInSight{};
	std::vector<PurgeZoneInfo*> pPurgeZonesInSight{};
	auto vEntitiesInFOV = GetEntitiesInFOV(); //uses m_pInterface->Fov_GetEntityByIndex(...)
	for (auto& e : vEntitiesInFOV)
	{
		switch (e.Type)
		{
		case	eEntityType::ENEMY:
			HandleEnemy(e, pEnemiesInSight);
			break;
		case eEntityType::ITEM:
			HandleItem(e, pItemsInSight);
			break;
		case eEntityType::PURGEZONE:
			HandlePurgeZone(e, pPurgeZonesInSight);
			break;
		default:
			break;
		}
	}

	//UPDATE Bb
	m_pB->ChangeData("Items", pItemsInSight);
	m_pB->ChangeData("Enemies", pEnemiesInSight);
	m_pB->ChangeData("PurgeZones", pPurgeZonesInSight);


}

void Plugin::HandleEnemy(const EntityInfo& eInfo, std::vector<EnemyInfo*>& pEnemiesInSight)
{
	pEnemiesInSight.push_back(new EnemyInfo());
	m_pInterface->Enemy_GetInfo(eInfo, *pEnemiesInSight.back());
}

void Plugin::HandleItem(EntityInfo& eInfo, std::vector<std::pair<EntityInfo, ItemInfo*>>& pItemsInSight)
{
	
	ItemInfo pItemInfo{};
	m_pInterface->Item_GetInfo(eInfo, pItemInfo);
	//std::cout << "Item:" << eInfo.Location.x << ", " << eInfo.Location.y << "---Type: " << " ---EntityHash: " << eInfo.EntityHash << std::endl;
	pItemsInSight.push_back(std::pair<EntityInfo, ItemInfo*>(eInfo, new ItemInfo(pItemInfo)));

}

void Plugin::HandlePurgeZone(const EntityInfo& eInfo, std::vector<PurgeZoneInfo*>& pPurgeZones)
{
	pPurgeZones.push_back(new PurgeZoneInfo());
	m_pInterface->PurgeZone_GetInfo(eInfo, *pPurgeZones.back());
	pPurgeZones.back()->Radius += 10;
	//std::cout << "Purge Zone in FOV:" << eInfo.Location.x << ", " << eInfo.Location.y << " ---EntityHash: " << eInfo.EntityHash << "---Radius: " << zoneInfo.Radius << std::endl;
}

void Plugin::HandleItemManagement()
{
	//INVENTORY USAGE DEMO
	//********************

	if (m_GrabItem)
	{
		ItemInfo item;
		//Item_Grab > When DebugParams.AutoGrabClosestItem is TRUE, the Item_Grab function returns the closest item in range
		//Keep in mind that DebugParams are only used for debugging purposes, by default this flag is FALSE
		//Otherwise, use GetEntitiesInFOV() to retrieve a vector of all entities in the FOV (EntityInfo)
		//Item_Grab gives you the ItemInfo back, based on the passed EntityHash (retrieved by GetEntitiesInFOV)
		if (m_pInterface->Item_Grab(GetEntitiesInFOV()[0], item))
		{
			//Once grabbed, you can add it to a specific inventory slot
			//Slot must be empty
			m_pInterface->Inventory_AddItem(0, item);
		}
	}

	//if (m_UseItem)
	//{
	//	//Use an item (make sure there is an item at the given inventory slot)
	//	m_pInterface->Inventory_UseItem(0);
	//}

	//if (m_RemoveItem)
	//{
	//	//Remove an item from a inventory slot
	//	m_pInterface->Inventory_RemoveItem(0);
	//}

	m_RemoveItem = false;
	m_UseItem = false;
	m_GrabItem = false;
}

void Plugin::DebugGrid()
{
	for (size_t i{}; i < m_pWayPoints.size(); i++)
	{
		if(!m_pWayPoints.at(i).first)
			m_pInterface->Draw_Point(m_pWayPoints.at(i).second, 3, Vector3{ 1,0,0 });
		else 
			m_pInterface->Draw_Point(m_pWayPoints.at(i).second, 3, Vector3{ 0,1,0 });
	}

	//GridReset
	size_t counter{};
	for (size_t i{}; i < m_pWayPoints.size(); i++)
	{
		if (m_pWayPoints.at(i).first)
			counter++;

	}
	if (counter >= m_pWayPoints.size()-1)
	{
		for (size_t i{}; i < m_pWayPoints.size(); i++)
		{
			m_pWayPoints.at(i).first = false;
		}
	}
}

void Plugin::InitBehavior()
{

	for (size_t i{ 0 }; i < m_pInterface->Inventory_GetCapacity(); i++)
	{
		m_pInventory[static_cast<unsigned char>(i)] = nullptr;
	}

	m_pBehaviors["Seek"] = new Seek();
	m_pBehaviors["Wander"] = new Wander();
	m_pBehaviors["Face"] = new Face();
	m_pBehaviors["Evade"] = new Evade();
	m_pBehaviors["FaceSeek"] = new FaceSeek();
	
	m_pBT = new BehaviorTree(m_pB,
		new BehaviorSelector(
			{
				new BehaviorSequence(
				{
						new BehaviorConditional(HasGarbage),
						new BehaviorAction(DestroyGarbage),
				}),
				new BehaviorSequence(
				{
						new BehaviorConditional(HasGun),
						new BehaviorConditional(IsAimingAtEnemy),
						new BehaviorAction(Shoot),
				}),
				new BehaviorSequence(
				{
						//Hasgun 1st, less calculations used if has no gun
						new BehaviorConditional(HasGun),
						new BehaviorConditional(IsInDanger),
						new BehaviorAction(AimAtEnemy),
					}),
				new BehaviorSelector(
					{
						new BehaviorSequence
						({
							new BehaviorConditional(IsInHouse),
							new BehaviorConditional(IsCloseToPurgeZone),
							new BehaviorAction(EscapePurgeZone),
						}),
						new BehaviorSequence
						({
							new BehaviorConditional(IsCloseToPurgeZone),
							new BehaviorAction(EscapePurgeZone),
						}),
					}),
				//eNEMYhANDLING
				new BehaviorSequence(
				{
							//Hasgun 1st, less calculations used if has no gun
						new BehaviorConditional(HasGun),
						new BehaviorConditional(IsBitten),
						new BehaviorAction(Turn),
				}),

				
				new BehaviorSequence(
					{
						new BehaviorConditional(IsCloseToBorder),
						new BehaviorAction(GoBack),
					}),
new BehaviorSelector(
	{

		//Item Usage
		new BehaviorSequence(
			{
				new BehaviorConditional(IsHungry),
				new BehaviorConditional(HasItemOfAgentState),
				new BehaviorAction(UseItemOfType),
			}),

		new BehaviorSequence(
			{
				new BehaviorConditional(IsInjured),
				new BehaviorConditional(HasItemOfAgentState),
				new BehaviorAction(UseItemOfType),
			}),




	}),

	//Picking up items
	new BehaviorSelector(
		{

			new BehaviorSequence(
				{
					new BehaviorConditional(HasFreeSlot),
					new BehaviorConditional(IsNearItems),
					new BehaviorConditional(IsHungry),
					new BehaviorConditional(IsItemOfTypeNearby),
					new BehaviorAction(GetItemOfType),
				}),

				new BehaviorSequence(
				{
					new BehaviorConditional(HasFreeSlot),
					new BehaviorConditional(IsNearItems),
					new BehaviorConditional(IsInjured),
					new BehaviorConditional(IsItemOfTypeNearby),
					new BehaviorAction(GetItemOfType),
				}),

			new BehaviorSequence(
				{
					new BehaviorConditional(HasFreeSlot),
					new BehaviorConditional(IsNearItems),
					new BehaviorAction(GetItem),
				}),


		}),

		//Wander after leaving purgezone
		new BehaviorSequence(
			{
				new BehaviorConditional(IsWanderActive),
				new BehaviorAction(ChangeToWander),
			}),

		//House exploring
		new BehaviorSelector(
			{

				new BehaviorSequence(
				{
					new BehaviorConditional(LeftHouse),
					new BehaviorAction(ChangeToWander),
				}),
				new BehaviorSequence(
				{

					new BehaviorConditional(IsHouseExplored),
					new BehaviorAction(LeaveHouse),
				}),
				new BehaviorSequence(
				{
					new BehaviorConditional(IsNearHouse),
					new BehaviorAction(EnterHouse),
				}),
				new BehaviorSequence(
				{
					new BehaviorConditional(GoingInside),
					new BehaviorAction(ChangeToSeek),
				}),
				new BehaviorSequence
				({
					new BehaviorConditional(IsInHouse),
					new BehaviorConditional(IsForAWhile),
					new BehaviorAction(EscapeHouse),
				}),
			}),
		
				new BehaviorSequence(
				{
					new BehaviorConditional(ExploredWaypoint),
					new BehaviorAction(FollowGrid),
				}),
				new BehaviorAction(ChangeToSeek),
				
			}));

	
}
SteeringPlugin_Output Plugin::HandleSteering(const float dt)
{
	SteeringPlugin_Output steering{};
	std::string mapID{};
	Vector2 target{};
	//use behavior string to set correct behavior
	m_pB->GetData("Behavior", mapID);
	m_pB->GetData("Target", m_Target);

	//Reset wanderangle to curr Agent angle
	if (mapID != std::string{ "Wander" })
	{
		static_cast<Wander*>(m_pBehaviors["Wander"])->UpdateWanderAngle(ToDegrees(m_pInterface->Agent_GetInfo().Orientation - static_cast<float>(M_PI)/2));
	}

	m_pBehaviors[mapID]->SetTarget(m_Target);
	steering.AutoOrient = true; //Setting AutoOrientate to TRue overrides the AngularVelocity
	steering = m_pBehaviors[mapID]->CalculateSteering(dt, m_pInterface);
	steering.RunMode = m_CanRun; //If RunMode is True > MaxLinSpd is increased for a limited time (till your stamina runs out)
	if (m_pInterface->Agent_GetInfo().WasBitten && m_pInterface->Agent_GetInfo().Stamina >= 5)
		m_Run = true;
	if (m_pInterface->Agent_GetInfo().Stamina <= 0)
		m_Run = false;
		steering.RunMode = m_Run;


	//std::cout << "Behavior: " << mapID << std::endl;

	return steering;
}

void Plugin::HandleTimers(const float dt)
{
	bool turning{};
	m_pB->GetData("Turning", turning);
	if (turning || m_pInterface->Agent_GetInfo().IsInHouse)
	{
		m_Cooldown += dt;
		if (m_Cooldown >= 5)
			m_Cooldown = 0;
		if (turning && m_Cooldown >= 2.f)
		{
			m_Cooldown = 0;
			m_pB->ChangeData("Turning", false);
		}
	}


	//Wander functionality
	float wanderTimer{};
	m_pB->GetData("WanderTimer", wanderTimer);

	if (wanderTimer > FLT_EPSILON)
	{
		wanderTimer -= dt;
		m_pB->ChangeData("WanderTimer", wanderTimer);
	}

}

void Plugin::HandleHouses()
{
	std::vector<HouseInfo*> vHousesInFOV{};
	std::vector<HouseInfo*> pExploredHouses{};
	m_pB->GetData("ExploredHouses", pExploredHouses);
	if (pExploredHouses.size() >= 6)
	{
		pExploredHouses.clear();
		m_pB->ChangeData("ExploredHouses", pExploredHouses);
	}


	if (!pExploredHouses.empty())
	{
		//std::cout << "THERE IS HOUSE EXPLORED" << std::endl;
	}
	for (size_t i{}; i < GetHousesInFOV().size(); i++)
	{
		//Check for yet explored houses
		HouseInfo* pHouse{ new HouseInfo(GetHousesInFOV()[i]) };
		auto it{ std::find_if(pExploredHouses.cbegin(), pExploredHouses.cend(), [&pHouse](const HouseInfo* pExploredHouse) {return int(pExploredHouse->Center.x) == int(pHouse->Center.x) && int(pExploredHouse->Center.y) == int(pHouse->Center.y); }) };
		if (it == pExploredHouses.cend())
		{
			vHousesInFOV.push_back(pHouse);//uses m_pInterface->Fov_GetHouseByIndex(...) //can loop over all gethouses vector and convert to pointers but seems unnecessary
		}
		else
		{
			//std::cout << "THIS IS HOUSE EXPLORED" << std::endl;
		}
	}
	m_pB->ChangeData("Houses", vHousesInFOV);
	/*if(!vHousesInFOV.empty())
		std::cout << "House in FOV with Size: " << vHousesInFOV.back()->Size.x << ", " << vHousesInFOV.back()->Size.y << std::endl
		<< "and center: " << vHousesInFOV.back()->Center.x << ", " << vHousesInFOV.back()->Center.y << std::endl;*/
}
