/*=============================================================================*/
// Copyright 2020-2021 Elite Engine
/*=============================================================================*/
// Behaviors.h: Implementation of certain reusable behaviors for the BT version of the Agario Game
/*=============================================================================*/
#ifndef ELITE_APPLICATION_BEHAVIOR_TREE_BEHAVIORS
#define ELITE_APPLICATION_BEHAVIOR_TREE_BEHAVIORS
//-----------------------------------------------------------------
// Includes & Forward Declarations
//-----------------------------------------------------------------
#include "../inc/EliteMath/EMath.h"
#include "EBehaviorTree.h"
#include "SteeringBehaviors.h"


//-----------------------------------------------------------------
//helperfunctions forward decl.
//-----------------------------------------------------------------
bool ContainsItemOfType(const std::vector<std::pair<EntityInfo, ItemInfo*>>& pItemsInRange, const eItemType requiredType);
bool ContainsItemOfType(const std::unordered_map<unsigned char, ItemInfo*>& pItemsInInventory, const eItemType requiredType); //overloading for inventory
bool IsCloseToCenter(const AgentInfo* pAgent, const std::vector<HouseInfo*>& pItemsInRange);
bool IsEfficientToUse(unsigned char idx, IExamInterface* pInterface, AgentInfo* pAgent);
bool IsLineSphereIntersection(const AgentInfo* pAgent, const EnemyInfo* pEnemy);
EnemyInfo* GetEnemyByPriority(const AgentInfo*pAgent, const std::vector<EnemyInfo*>& pEnemies);

unsigned char GetIdxOfInventoryItem(const std::unordered_map<unsigned char, ItemInfo*>& pItemsInInventory, eItemType wantedType);
unsigned char GetIdxOfInventoryItem(const std::unordered_map<unsigned char, ItemInfo*>& pItemsInInventory, eItemType wantedType, IExamInterface* pInterface);

unsigned char GetFirstFreeInventoryIdx(const std::unordered_map<unsigned char, ItemInfo*>& pItemsInInventory);
std::vector<unsigned char> GetAllInventoryItemsOfType(const std::unordered_map<unsigned char, ItemInfo*>& pItemsInInventory, eItemType wantedType);
//std::vector<std::pair<unsigned char, ItemInfo*>> GetAllInventoryItemsOfType(const AgentInfo* pAgent, const std::unordered_map<);


std::pair<EntityInfo, ItemInfo*> GetClosestItem(const AgentInfo* pAgent, const std::vector<std::pair<EntityInfo, ItemInfo*>>& pItemsInRange);
std::pair<EntityInfo, ItemInfo*> GetClosestItemOfType(const AgentInfo* pAgent, const std::vector<std::pair<EntityInfo, ItemInfo*>>& pItemsInRange, const eItemType requiredType);
std::unordered_map<int, std::pair<bool, Vector2>> GetAllAvailableWaypoints(const AgentInfo* pAgent, std::unordered_map<int, std::pair<bool, Vector2>> waypoints);


//-----------------------CONDITIONALS-------------------------
bool ExploredWaypoint(Elite::Blackboard* pB)
{
	AgentInfo* pAgent{ nullptr };
	Vector2 target{};
	auto dataAvailable{ pB->GetData("Agent", pAgent) && pB->GetData("Target", target) };
	if (!dataAvailable || !pAgent)
		return false;
	if (DistanceSquared(pAgent->Position, target) < 10.f || target == Vector2{ 0,0 })
		return true;
	return false;
}

bool IsCloseToPurgeZone(Elite::Blackboard* pB)
{
	AgentInfo* pAgent{ nullptr };
	std::vector<PurgeZoneInfo*> pPurgeZone{};
	auto dataAvailable{ pB->GetData("Agent", pAgent) && pB->GetData("PurgeZones",pPurgeZone) };
	if (!dataAvailable || !pAgent || pPurgeZone.empty())
		return false;
	
	auto it{ std::min_element(pPurgeZone.cbegin(), pPurgeZone.cend(),[&pAgent](const PurgeZoneInfo* pPZ1, const PurgeZoneInfo* pPZ2) {return DistanceSquared(pPZ1->Center, pAgent->Position) < DistanceSquared(pPZ2->Center, pAgent->Position);}) };
	if (it == pPurgeZone.cend())
		return false;

	if (DistanceSquared((*it)->Center, pAgent->Position) >= ((*it)->Radius + 10)* ((*it)->Radius + 10))
		return false;

	pB->ChangeData("Target", (*it)->Center);
	pB->ChangeData("WanderTimer", 5.f);
	return true;
}


bool IsCloseToBorder(Elite::Blackboard* pB)
{
	AgentInfo* pAgent{ nullptr };
	IExamInterface* pInterface{};
	bool closeToBorder{ false };
	auto dataAvailable{ pB->GetData("Agent", pAgent) && pB->GetData("pInterface",pInterface) && pB->GetData("CloseToBorder", closeToBorder)};
	if (!dataAvailable || !pAgent)
		return false;

	if (closeToBorder)
		return true;

	Vector2 worldC{ pInterface->World_GetInfo().Center };
	Vector2 worldS{ pInterface->World_GetInfo().Dimensions };
	bool isAtBorder{false};
	if (pAgent->Position.x <= worldC.x - worldS.x / 2)
	{
		pB->ChangeData("Target", Vector2{ pAgent->Position.x + 5, pAgent->Position.y });
		isAtBorder = true;
	}
	else if (pAgent->Position.x >= worldC.x + worldS.x / 2)
	{
		pB->ChangeData("Target", Vector2{ pAgent->Position.x - 5, pAgent->Position.y });
		isAtBorder = true;
	}
	else if (pAgent->Position.y <= worldC.y - worldS.y / 2)
	{
		pB->ChangeData("Target", Vector2{ pAgent->Position.x , pAgent->Position.y + 5 });
		isAtBorder = true;
	}
	else if (pAgent->Position.y >= worldC.y + worldS.y / 2)
	{
		pB->ChangeData("Target", Vector2{ pAgent->Position.x , pAgent->Position.y - 5 });
		isAtBorder = true;
	}
	if(isAtBorder)
		pB->ChangeData("CloseToBorder", true);

	return isAtBorder;
}
//-------agent IS states-------
bool IsHungry(Elite::Blackboard* pB)
{
	AgentInfo* pAgent{ nullptr };
	auto dataAvailable{ pB->GetData("Agent", pAgent)};
	if (!dataAvailable || !pAgent)
		return false;
	if (pAgent->Energy <= 7.1f)
	{
		//Add type for "Get/Has ItemOfAgentState functionality
		pB->ChangeData("WantedType", eItemType::FOOD);
		return true;
	}
	return false;
}

bool IsWanderActive(Elite::Blackboard* pB)
{
	float wanderTimer{};

	auto dataAvailable{  pB->GetData("WanderTimer", wanderTimer) };
	if (!dataAvailable)
		return false;
	if (wanderTimer <= 0 )
	{
		//Add type for "Get/Has ItemOfAgentState functionality
		pB->ChangeData("WanderTimer", 0.f);
		return false;
	}
	return true;
}


bool IsInjured(Elite::Blackboard* pB)
{
	AgentInfo* pAgent{ nullptr };
	auto dataAvailable{ pB->GetData("Agent", pAgent) };
	if (!dataAvailable || !pAgent)
		return false;

	if (pAgent->Health <= 8.1f)
	{
		//Add type for "Get/Has ItemOfAgentState functionality
		pB->ChangeData("WantedType", eItemType::MEDKIT);
		return true;
	}
	return false;
}
bool IsBitten(Elite::Blackboard* pB)
{
	AgentInfo* pAgent{ nullptr };
	bool turning{};
	auto dataAvailable{ pB->GetData("Agent", pAgent) && pB->GetData("Turning", turning) };
	if (!dataAvailable || !pAgent)
		return false;
	if (turning)
		return true;
	if (pAgent->WasBitten)
	{
		pB->ChangeData("Turning",true);
		return true;
	}
	return false;
}

//----------------------------

//-------ItemManagement-------
bool HasGarbage(Elite::Blackboard* pB)
{
	std::unordered_map<unsigned char, ItemInfo*>* pInventory{};
	auto dataAvailable{ pB->GetData("Inventory",pInventory) };
	if (!dataAvailable || pInventory->empty())
		return false;
	if (ContainsItemOfType(*pInventory, eItemType::GARBAGE))
	{
		return true;
	}
	return false;
}
bool HasItemOfAgentState(Elite::Blackboard* pB)
{
	std::unordered_map<unsigned char, ItemInfo*>* pInventory{};
	eItemType wantedType{};
	auto dataAvailable{ pB->GetData("WantedType", wantedType) && pB->GetData("Inventory",pInventory) };
	if (!dataAvailable || pInventory->empty())
		return false;
	if (ContainsItemOfType(*pInventory, wantedType))
	{
		return true;
	}
	return false;
}
bool HasGun(Elite::Blackboard* pB)
{
	std::unordered_map<unsigned char, ItemInfo*>* pInventory{};
	auto dataAvailable{  pB->GetData("Inventory",pInventory) };
	if (!dataAvailable || pInventory->empty())
		return false;
	if (ContainsItemOfType(*pInventory, eItemType::PISTOL))
	{
		//std::cout << "HAS GUN" << std::endl;
		return true;
	}

	pB->ChangeData("Turning", false);
	return false;
}

//-------------------------

//-------ItemFinding-------

bool HasFreeSlot(Elite::Blackboard* pB)
{
	AgentInfo* pAgent{ nullptr };
	std::vector<std::pair<EntityInfo, ItemInfo*>> pItems{ };
	IExamInterface* pInterface{ nullptr };
	std::unordered_map<unsigned char, ItemInfo*>* pInventory{};

	auto dataAvailable{ pB->GetData("Agent", pAgent) && pB->GetData("Items", pItems) && pB->GetData("pInterface", pInterface) && pB->GetData("Inventory", pInventory)};
	if (!dataAvailable || pItems.empty() || !pAgent || !pInterface)
		return false;

	auto it = std::find_if(pInventory->cbegin(), pInventory->cend(), [](const std::pair<unsigned char, ItemInfo*>& pItem) {return pItem.second == nullptr; });
	if (it == pInventory->cend())
		return false;
	return true;
}

bool IsNearItems(Elite::Blackboard* pB)
{
	AgentInfo* pAgent{ nullptr };
	std::vector<std::pair<EntityInfo, ItemInfo*>> pItems{ };
	IExamInterface* pInterface{ nullptr };


	auto dataAvailable{ pB->GetData("Agent", pAgent) && pB->GetData("Items", pItems) && pB->GetData("pInterface", pInterface) };
	if (!dataAvailable || pItems.empty() || !pAgent || !pInterface)
		return false;


	return true;
}

//To save codelines not having an IsFoodNearby, IsMedkitNearby, IsGunNearby... easier to expand
bool IsItemOfTypeNearby(Elite::Blackboard* pB)
{
	AgentInfo* pAgent{ nullptr };
	std::vector<std::pair<EntityInfo, ItemInfo*>> pItems{};
	eItemType wantedType{};
	IExamInterface* pInterface{ nullptr };

	auto dataAvailable{ pB->GetData("Agent", pAgent) && pB->GetData("Items", pItems) && pB->GetData("pInterface", pInterface) && pB->GetData("WantedType", wantedType)};
	if (!dataAvailable || pItems.empty() || !pAgent || !pInterface)
		return Failure;

	//Returns true if food is in FOV
	//std::cout << "ITEMS FOUND: " << pItems.size() << std::endl;

	return ContainsItemOfType(pItems, wantedType);
}
//------------------------------
//-------House management-------
bool IsInHouse(Elite::Blackboard* pB)
{
	AgentInfo* pAgent{ nullptr };
	std::vector<HouseInfo*> pHouses{ nullptr };
	auto dataAvailable{ pB->GetData("Agent", pAgent) && pB->GetData("Houses", pHouses) };
	if (!dataAvailable || pHouses.empty() || !pAgent)
		return false;
	if (pAgent->IsInHouse)
	{
		return true;
	}
	return false;
}
bool IsForAWhile(Elite::Blackboard* pB)
{
	float* cooldown{new float()};
	AgentInfo* pAgent{ nullptr };
	std::vector<HouseInfo*> pHouses{ nullptr };
	auto dataAvailable{ pB->GetData("Agent", pAgent) && pB->GetData("Houses", pHouses) && pB->GetData("Cooldown", cooldown) };
	if (!dataAvailable || pHouses.empty() || !pAgent)
		return false;
	if (*cooldown >= 5.f)
	{
		pB->ChangeData("Cooldown", 0);
		return true;
	}
	return false;
}
bool IsNearHouse(Elite::Blackboard* pB)
{
	AgentInfo* pAgent{ nullptr };
	std::vector<HouseInfo*> pHouses{ nullptr };
	bool goingInside{};
	auto dataAvailable{ pB->GetData("Agent", pAgent) && pB->GetData("Houses", pHouses) &&pB->GetData("GoingInside",goingInside) };
	if (!dataAvailable || pHouses.empty() || !pAgent)
		return false;

	//save pos outside house

	return true;
}

bool IsHouseExplored(Elite::Blackboard* pB)
{
	AgentInfo* pAgent{ nullptr };
	std::vector<HouseInfo*> pHouses{ nullptr };
	bool isLeavingHouse{};
	auto dataAvailable{ pB->GetData("Houses", pHouses) && pB->GetData("Agent", pAgent) && pB->GetData("LeavingHouse",isLeavingHouse) };

	if (!dataAvailable || !pAgent)
		return false;

	if (isLeavingHouse)
		return true;
	return IsCloseToCenter(pAgent, pHouses);
}

bool LeftHouse(Elite::Blackboard* pB)
{
	AgentInfo* pAgent{ nullptr };
	Vector2 outsidePos{};
	bool isLeavingHouse{};
	auto dataAvailable{ pB->GetData("Agent", pAgent) && pB->GetData("OutsidePos", outsidePos) && pB->GetData("LeavingHouse",isLeavingHouse) };
	if (!dataAvailable || !pAgent)
		return false;	
	if ((DistanceSquared(pAgent->Position, outsidePos) < 2.5f) && isLeavingHouse)
	{
		pB->ChangeData("LeavingHouse", false);
		pB->ChangeData("GoingInside", false);
		return true;
	}
	//save pos outside house
	return false;
}

bool GoingInside(Blackboard* pB)
{
	
	std::vector<HouseInfo*> pHouses{ nullptr };
	bool goingInside{};
	auto dataAvailable{  pB->GetData("GoingInside",goingInside) };
	if (!dataAvailable)
		return false;

	if (goingInside)
		return true;
	//save pos outside house

	return false;
}
//--------------------------
//-------Enemies-------
bool IsInDanger(Elite::Blackboard* pB)
{
	AgentInfo* pAgent{ nullptr };
	std::vector<EnemyInfo*> pEnemies{};
	auto dataAvailable{ pB->GetData("Agent", pAgent) && pB->GetData("Enemies", pEnemies) };
	if (!dataAvailable || !pAgent || pEnemies.empty())
		return false;

	for (EnemyInfo* pEnemy : pEnemies)
	{
		if (DistanceSquared(pAgent->Position, pEnemy->Location) <= 256)
		{
			return true;
		}
	}

	return false;
}
bool IsAimingAtEnemy(Elite::Blackboard* pB)
{
	AgentInfo* pAgent{ nullptr };
	std::vector<EnemyInfo*> pEnemies{};
	auto dataAvailable{ pB->GetData("Agent", pAgent) && pB->GetData("Enemies", pEnemies) };
	if (!dataAvailable || !pAgent || pEnemies.empty())
		return false;
	EnemyInfo* pDangerousEnemy{ GetEnemyByPriority(pAgent, pEnemies) };
	//Endpoint of the shooting line trace
	Vector2 A{ pAgent->Position };
	Vector2 B{ pAgent->Position.x + pAgent->FOV_Range * cosf(pAgent->Orientation), pAgent->Position.y + pAgent->FOV_Range * sinf(pAgent->Orientation) };
	if( DistanceSquared(B, pDangerousEnemy->Location) < 1)
		return true;
	else if(IsLineSphereIntersection(pAgent, pDangerousEnemy))
	{
		//sphere intersection
		return true;
	}
	return false;

}
//-----------------------------------------------------------------



//---------------------------ACTIONS-------------------------------
BehaviorState ChangeToEvade(Elite::Blackboard* pBlackboard)
{
	AgentInfo* pAgent{ nullptr };
	auto dataAvailable{ pBlackboard->GetData("Agent", pAgent) };
	if (!dataAvailable || !pAgent )
		return Failure;

	pBlackboard->ChangeData("Behavior", std::string{ "Evade" });

	return Success;
}

BehaviorState RunFromEnemy(Elite::Blackboard* pBlackboard)
{
	AgentInfo* pAgent{ nullptr };
	std::vector<EnemyInfo*> pEnemies{};
	auto dataAvailable{ pBlackboard->GetData("Agent", pAgent) && pBlackboard->GetData("Enemies", pEnemies) };
	if (!dataAvailable || !pAgent || pEnemies.empty())
		return Failure;
	
	pBlackboard->ChangeData("Target", GetEnemyByPriority(pAgent, pEnemies)->Location);

	pBlackboard->ChangeData("Behavior", std::string{ "Evade" });

	return Success;
}


BehaviorState ChangeToWander(Elite::Blackboard* pBlackboard)
{
	AgentInfo* pAgent = nullptr;
	auto dataAvailable = pBlackboard->GetData("Agent", pAgent);

	if (!dataAvailable || !pAgent)
		return Failure;

	Wander wander{};
	
	pBlackboard->ChangeData("Behavior", std::string{ "Wander" }) ;
	//std::cout << "wander" << std::endl;

	return Success;
}

BehaviorState ChangeToSeek(Elite::Blackboard* pBlackboard)
{
	AgentInfo* pAgent{ nullptr };
	Vector2 seekTarget{};
	auto dataAvailable{ pBlackboard->GetData("Agent", pAgent)};

	if (!dataAvailable || !pAgent)
		return Failure;
	pBlackboard->ChangeData("Behavior", std::string{ "Seek" });
	//std::cout << "seeking to food" << std::endl;
	return Success;
}

BehaviorState GoBack(Elite::Blackboard* pBlackboard)
{
	AgentInfo* pAgent{ nullptr };
	Vector2 target{};
	auto dataAvailable{ pBlackboard->GetData("Agent", pAgent) && pBlackboard->GetData("Target", target) };

	if (!dataAvailable || !pAgent)
		return Failure;
	if (DistanceSquared(target, pAgent->Position) < 10)
	{
		pBlackboard->ChangeData("CloseToBorder", false);
		return Failure;
	}

	pBlackboard->ChangeData("Behavior", std::string{ "Seek" });
	//std::cout << "seeking to food" << std::endl;
	return Success;
}

BehaviorState ChangeToFace(Elite::Blackboard* pBlackboard)
{
	AgentInfo* pAgent{ nullptr };
	Vector2 seekTarget{};
	auto dataAvailable{ pBlackboard->GetData("Agent", pAgent) };

	if (!dataAvailable || !pAgent)
		return Failure;
	pBlackboard->ChangeData("Behavior", std::string{ "Face" });
	//std::cout << "seeking to food" << std::endl;
	return Success;
}

BehaviorState FollowGrid(Elite::Blackboard* pBlackboard)
{
	AgentInfo* pAgent{ nullptr };
	Vector2 target{};
	std::unordered_map<int, std::pair<bool, Vector2>>* pWayPoints{nullptr};
	
	auto dataAvailable{ pBlackboard->GetData("Agent", pAgent) && pBlackboard->GetData("Waypoints", pWayPoints)};
	if (!dataAvailable || !pAgent || !pWayPoints)
		return Failure;

	std::unordered_map<int, std::pair<bool, Vector2>> pAvailableWaypoints{ GetAllAvailableWaypoints(pAgent, *pWayPoints) };

	//need to edit bool of the mapelement
	auto it = std::min_element(pAvailableWaypoints.begin(), pAvailableWaypoints.end(), [&pAgent](const std::pair<int, std::pair<bool, Vector2>>& waypointA, const std::pair<int, std::pair<bool, Vector2>>& waypointB)
		{
			return DistanceSquared(pAgent->Position, waypointA.second.second) < DistanceSquared(pAgent->Position, waypointB.second.second);
		});
	if (it == pAvailableWaypoints.cend())
		return Failure;
	if (DistanceSquared(pAgent->Position, pWayPoints->at(it->first).second) < 15.f)
		pWayPoints->at(it->first).first = true;

	pBlackboard->ChangeData("Behavior", std::string{ "Seek" });
	pBlackboard->ChangeData("Target", it->second.second);
	//std::cout << "seeking to food" << std::endl;
	return Success;

}

BehaviorState GetItem(Elite::Blackboard* pBlackboard)
{
	AgentInfo* pAgent{ nullptr };
	std::vector<std::pair<EntityInfo, ItemInfo*>> pItems{};
	std::unordered_map<unsigned char, ItemInfo*>* pInventory{};
	IExamInterface* pInterface{ nullptr };


	auto dataAvailable{ pBlackboard->GetData("Agent", pAgent) 
		&& pBlackboard->GetData("Items", pItems) 
		&& pBlackboard->GetData("pInterface", pInterface)
		&& pBlackboard->GetData("Inventory", pInventory)};
	if (!dataAvailable || pItems.empty() || !pAgent || !pInterface)
		return Failure;

	std::pair<EntityInfo, ItemInfo*> closestItem = GetClosestItem(pAgent, pItems);
	if (!closestItem.second)
		return Failure;
	//Data Updating
	//std::cout << "GETTING ITEM" << std::endl;
	pBlackboard->ChangeData("Behavior", std::string{ "Seek" });
	pBlackboard->ChangeData("Target", closestItem.second->Location);


	size_t amountOfItem{ GetAllInventoryItemsOfType(*pInventory, closestItem.second->Type).size() };
	if (closestItem.second->Type!=eItemType::PISTOL && amountOfItem >= 2)
		return Failure;
	//try to grab item
	if ( pInterface->Item_Grab(closestItem.first, *closestItem.second))
	{
		pInterface->Inventory_AddItem(GetFirstFreeInventoryIdx(*pInventory), *closestItem.second);
		pInventory->operator[](static_cast<unsigned char>(GetFirstFreeInventoryIdx(*pInventory))) = closestItem.second;

	}
	return Success;

}

//To save codelines not having a GetFood, GetMedkit, GetGun... easy to expand and easier to read, no useless code recycling
//If  Agent is hungry prioritize food
BehaviorState GetItemOfType(Elite::Blackboard* pBlackboard)
{
	AgentInfo* pAgent{ nullptr };
	eItemType wantedType{};
	std::vector<std::pair<EntityInfo, ItemInfo*>> pItems{ };
	std::unordered_map<unsigned char, ItemInfo*>* pInventory{};
	IExamInterface* pInterface{ nullptr };


	auto dataAvailable{ pBlackboard->GetData("Agent", pAgent) 
		&& pBlackboard->GetData("Items", pItems)
		&& pBlackboard->GetData("pInterface", pInterface)
		&& pBlackboard->GetData("Inventory", pInventory) 
		&& pBlackboard->GetData("WantedType", wantedType)};
	if (!dataAvailable || pItems.empty() || !pAgent || !pInterface)
		return Failure;

	//Get the closest entity and item matching a specific eItemType (Prioritization hunger>hurt>gun) (see Ishungry, IsInjured...)
	std::pair<EntityInfo, ItemInfo*> closestItemOfType{ GetClosestItemOfType(pAgent, pItems, wantedType) };
	if (!closestItemOfType.second)
		return Failure;
	//Data Updating
	//std::cout << "GETTING FOOD" << std::endl;
	pBlackboard->ChangeData("Behavior", std::string{ "Seek" });
	pBlackboard->ChangeData("Target", closestItemOfType.second->Location);
	


	size_t amountOfItem{ GetAllInventoryItemsOfType(*pInventory, wantedType).size() };
	if (wantedType != eItemType::PISTOL && amountOfItem >= 2)
		return Failure;

	//try to grab item
	if (pInterface->Item_Grab(closestItemOfType.first, *closestItemOfType.second))
	{
		
		pInterface->Inventory_AddItem(GetFirstFreeInventoryIdx(*pInventory), *closestItemOfType.second);
		pInventory->operator[](static_cast<unsigned char>(GetFirstFreeInventoryIdx(*pInventory))) = closestItemOfType.second;

	}
	return Success;
}


BehaviorState UseItemOfType(Elite::Blackboard* pBlackboard)
{
	AgentInfo* pAgent{ nullptr };
	eItemType wantedType{};
	std::unordered_map<unsigned char, ItemInfo*>* pInventory{};
	IExamInterface* pInterface{ nullptr };


	auto dataAvailable{ pBlackboard->GetData("Agent", pAgent)
		&& pBlackboard->GetData("pInterface", pInterface)
		&& pBlackboard->GetData("Inventory", pInventory)
		&& pBlackboard->GetData("WantedType", wantedType) };
	if (!dataAvailable || !pAgent || !pInterface)
		return Failure;
	//Get item of type from inventory and use! Drop if empty
	std::vector<unsigned char> indices = GetAllInventoryItemsOfType(*pInventory, wantedType);

	for (size_t i{ 0 }; i < indices.size(); i++)
	{
		//Only use the item if it's efficient to use (if item + current agent value <= max value
		if (IsEfficientToUse(indices[i], pInterface, pAgent))
		{
			pInterface->Inventory_UseItem(indices[i]);
			pInterface->Inventory_RemoveItem(indices[i]); //drop other items when used (always fully depleted in game on 1 usage)
			delete pInventory->at(indices[i]);
			pInventory->at(indices[i]) = nullptr;
			return Success;
		}
	}
	return Failure;
}

BehaviorState DestroyGarbage(Elite::Blackboard* pBlackboard)
{
	AgentInfo* pAgent{ nullptr };
	std::unordered_map<unsigned char, ItemInfo*>* pInventory{};
	IExamInterface* pInterface{ nullptr };


	auto dataAvailable{ pBlackboard->GetData("Agent", pAgent)
		&& pBlackboard->GetData("pInterface", pInterface)
		&& pBlackboard->GetData("Inventory", pInventory) };
	if (!dataAvailable || !pAgent || !pInterface)
		return Failure;
	//Get item of type from inventory and use! Drop if empty
	std::vector<unsigned char> indices = GetAllInventoryItemsOfType(*pInventory, eItemType::GARBAGE);
	for (size_t i{ 0 }; i < indices.size(); i++)
	{
		
		pInterface->Inventory_RemoveItem(indices[i]); //drop other items when used (always fully depleted in game on 1 usage)
		delete pInventory->at(indices[i]);
		pInventory->at(indices[i]) = nullptr;
		
	}
	return Success;

}

//Separate shooting

BehaviorState AimAtEnemy(Elite::Blackboard* pBlackboard)
{
	AgentInfo* pAgent{ nullptr };
	std::vector<EnemyInfo*> pEnemies{ nullptr };

	auto dataAvailable{ pBlackboard->GetData("Agent", pAgent)
		&& pBlackboard->GetData("Enemies", pEnemies) };
	if (!dataAvailable || pEnemies.empty())
		return Failure;

	//Aim at most dangerous enemy of the bunch
	EnemyInfo* pDangerousEnemy{ GetEnemyByPriority(pAgent, pEnemies) };
	if (pDangerousEnemy)
	{
		pBlackboard->ChangeData("Turning", false);

		pBlackboard->ChangeData("Behavior", std::string("Face"));
		pBlackboard->ChangeData("Target", pDangerousEnemy->Location);
		return Success;
	}
	return Failure;
}

BehaviorState Turn(Elite::Blackboard* pBlackboard)
{
	AgentInfo* pAgent{ nullptr };
	bool isTurning{};
	auto dataAvailable{ pBlackboard->GetData("Agent", pAgent)  && pBlackboard->GetData("Turning", isTurning)};
	if (!dataAvailable)
		return Failure;
	
	Vector2 target{ pAgent->Position.x + 2 * cosf(pAgent->Orientation - 3 * static_cast<float>(M_PI)), pAgent->Position.y + 2 * sinf(pAgent->Orientation - 3 * static_cast<float>(M_PI)) };
	pBlackboard->ChangeData("Behavior", std::string("FaceSeek"));
	pBlackboard->ChangeData("Target", target);
	return Success;
	
}

BehaviorState Shoot(Elite::Blackboard* pBlackboard)
{
	AgentInfo* pAgent{nullptr};
	std::unordered_map<unsigned char, ItemInfo*>* pInventory{};
	IExamInterface* pInterface{ nullptr };
	std::vector<EnemyInfo*> pEnemies{ nullptr };
	
	auto dataAvailable{ pBlackboard->GetData("Agent", pAgent)
		&& pBlackboard->GetData("pInterface", pInterface)
		&& pBlackboard->GetData("Inventory", pInventory)
		&& pBlackboard->GetData("Enemies", pEnemies)};
	if (!dataAvailable || !pInterface || pEnemies.empty())
		return Failure; 
	
	const unsigned char idx{ GetIdxOfInventoryItem(*pInventory, eItemType::PISTOL, pInterface) };
	if (!pInventory->at(idx))
		return Failure;
	//Is enemy still there
	EnemyInfo* pDangerousEnemy{ GetEnemyByPriority(pAgent, pEnemies) };
	if(pDangerousEnemy)
		pInterface->Inventory_UseItem(idx);

	//drop if weapon empty
	if (pInterface->Weapon_GetAmmo(*pInventory->at(idx)) <= 0)
	{
		pInterface->Inventory_RemoveItem(idx);
		delete pInventory->at(idx);
		pInventory->at(idx) = nullptr;
	}
	pBlackboard->ChangeData("GoingInside", false);

	return Success;
}

BehaviorState EnterHouse(Elite::Blackboard* pBlackboard)
{
	AgentInfo* pAgent{ nullptr };
	std::vector<HouseInfo*> pHouses{ nullptr };
	bool isEnteringHouse{};

	auto dataAvailable{ pBlackboard->GetData("Agent", pAgent) && pBlackboard->GetData("Houses", pHouses) && pBlackboard->GetData("GoingInside", isEnteringHouse) };
	if (!dataAvailable || pHouses.empty() || !pAgent)
		return Failure;

	//Data Updating
	Vector2 toHouse{ pHouses[0]->Center - pAgent->Position };
	Vector2 target{ pHouses[0]->Center };
	//go outside to op or bottom of house
	if(toHouse.y < 0)
		target.y -= pHouses[0]->Size.y/2 + 4;
	else if(toHouse.y>0)
		target.y += pHouses[0]->Size.y / 2 + 4;

	pBlackboard->ChangeData("Behavior", std::string{ "Seek" });
	pBlackboard->ChangeData("Target", pHouses[0]->Center);

	if (!isEnteringHouse)
		pBlackboard->ChangeData("OutsidePos", target);
	pBlackboard->ChangeData("GoingInside", true);
	return Success;
}

BehaviorState LeaveHouse(Elite::Blackboard* pBlackboard)
{
	Vector2 outsidePos{};
	AgentInfo* pAgent{ nullptr };
	std::vector<HouseInfo*> pHouses{ nullptr };
	std::vector<HouseInfo*> pExploredHouses{ nullptr };
	bool leavingHouse{};
	auto dataAvailable{ pBlackboard->GetData("Agent", pAgent) && pBlackboard->GetData("OutsidePos", outsidePos) && pBlackboard->GetData("Houses", pHouses) && pBlackboard->GetData("ExploredHouses", pExploredHouses) && pBlackboard->GetData("LeavingHouse",leavingHouse)};

	if (!pAgent)
		return Failure;
	if(!pHouses.empty())
		pExploredHouses.push_back(pHouses[0]);

	//std::cout << outsidePos.x << ", " << outsidePos.y << std::endl;
	//Data Updating
	if(!leavingHouse)
		pBlackboard->ChangeData("ExploredHouses", pExploredHouses);

	pBlackboard->ChangeData("LeavingHouse", true);
	pBlackboard->ChangeData("Behavior", std::string{ "Seek" });
	pBlackboard->ChangeData("Target", outsidePos);
	return Success;
}

BehaviorState EscapeHouse(Elite::Blackboard* pB)
{
	AgentInfo* pAgent{ nullptr };
	auto dataAvailable{ pB->GetData("Agent", pAgent)};
	if (!dataAvailable || !pAgent)
		return Failure;

	Vector2 dir{ pAgent->Position.x - 10, pAgent->Position.y };

	pB->ChangeData("Behavior", std::string{ "Seek" });

	pB->ChangeData("Target", dir);
	return Success;
}

BehaviorState EscapePurgeZone(Elite::Blackboard* pB)
{
	AgentInfo* pAgent{ nullptr };
	Vector2 target{};
	std::vector<PurgeZoneInfo*> pPurgeZone{};
	auto dataAvailable{ pB->GetData("Agent", pAgent) && pB->GetData("Target",target) && pB->GetData("PurgeZones",pPurgeZone) };
	if (!dataAvailable || !pAgent)
		return Failure;

	Vector2 dirToAgent{ pAgent->Position - target };
	dirToAgent = dirToAgent.GetNormalized();
	dirToAgent *= pPurgeZone[0]->Radius + 3;
	dirToAgent = pPurgeZone[0]->Center + dirToAgent;
	pB->ChangeData("Target", dirToAgent);
	pB->ChangeData("Behavior", std::string{ "Seek" });
	return Success;

}

//-----------------------------------------------------------------------







//-------------------------HELPER FUNCTIONS------------------------------
bool ContainsItemOfType(const std::vector<std::pair<EntityInfo, ItemInfo*>>& pItemsInRange, const eItemType requiredType)
{
	//objects in range collection
	//Get closest item
	auto it = std::find_if(pItemsInRange.cbegin(), pItemsInRange.cend(), [requiredType](const std::pair<EntityInfo, ItemInfo*> pItem) {return pItem.second->Type == requiredType; });
	if (it != pItemsInRange.cend())
		return true;
	return false;

}

bool ContainsItemOfType(const std::unordered_map<unsigned char, ItemInfo*>& pItemsInInventory, const eItemType wantedType)
{
	auto it = std::find_if(pItemsInInventory.cbegin(), pItemsInInventory.cend(), [wantedType](const std::pair<unsigned char, ItemInfo*>& pItem) 
		{
			if(pItem.second)
				return pItem.second->Type == wantedType; 
			return false;
		});
	if (it != pItemsInInventory.cend())
		return true;
	return false;
}
bool IsEfficientToUse(unsigned char idx, IExamInterface* pInterface, AgentInfo* pAgent)
{
	ItemInfo pItemInfo{ };
	pInterface->Inventory_GetItem(idx, pItemInfo);
	switch (pItemInfo.Type)
	{
	case eItemType::FOOD:
		if (pInterface->Food_GetEnergy(pItemInfo) + pAgent->Energy <= 10)
			return true;
			break;
	case eItemType::MEDKIT:
		if (pInterface->Medkit_GetHealth(pItemInfo) + pAgent->Health <= 10)
			return true;
		break;
	case eItemType::PISTOL:
		return true;
		break;

	default:
		break;
	}
	return false;
}



bool IsCloseToCenter(const AgentInfo* pAgent, const std::vector<HouseInfo*>& pHousesInRange)
{
	auto it = std::find_if(pHousesInRange.cbegin(), pHousesInRange.cend(), [&pAgent](const HouseInfo* pHouse) {return DistanceSquared(pHouse->Center, pAgent->Position) < 5.5f; });
	if (it != pHousesInRange.cend())
		return true;
	return false;
}

bool IsLineSphereIntersection(const AgentInfo* pAgent, const EnemyInfo* pEnemy)
{
	Vector2 rayO{ pAgent->Position };
	Vector2 rayD{ cosf(pAgent->Orientation -  static_cast<float>(M_PI) / 2), sinf(pAgent->Orientation - static_cast<float>(M_PI) / 2)};
	Vector2 center{pEnemy->Location};
	float radius{ pEnemy->Size };
	float radius2{ radius * radius };
	float a{ Dot(rayD,rayD) };
	float b{ Dot(2.f * rayD,(rayO - center)) };
	float c{ Dot(rayO - center, rayO - center) - radius2 };
	float discriminant{ (b * b) - (4 * a * c) };
	//discriminant <= 0 no intersection/touching
	if (discriminant <= 0) 
		return false;
	//intersection with enemy
	return true;

}



EnemyInfo* GetEnemyByPriority(const AgentInfo* pAgent, const std::vector<EnemyInfo*>& pEnemies)
{
	//Runners are more dangerous than other kinds, prioritize
	std::vector<EnemyInfo*> pEnemiesByPriority{};
	auto it{ std::find_if(pEnemies.cbegin(), pEnemies.cend(), [](const EnemyInfo* pEnemy) {return pEnemy->Type == eEnemyType::ZOMBIE_RUNNER; }) };

	if (it != pEnemies.cend())
		pEnemiesByPriority.push_back(*it);

	//Add all Runners items to foodvec
	while (it != pEnemies.cend())
	{
		it = std::find_if(std::next(it), pEnemies.cend(), [](const EnemyInfo* pEnemy){ return pEnemy->Type == eEnemyType::ZOMBIE_RUNNER;});
		if (it != pEnemies.cend())
			pEnemiesByPriority.push_back(*it);
	}
	if (!pEnemiesByPriority.empty())
	{
		return *std::min_element(pEnemiesByPriority.cbegin(), pEnemiesByPriority.cend(), [&pAgent](const EnemyInfo* pRunner, const EnemyInfo* pOtherRunner) { return DistanceSquared(pRunner->Location, pAgent->Position) < DistanceSquared(pOtherRunner->Location, pAgent->Position); });
	}
	//Normal/heavy enemies < Runners in prio
	return *std::min_element(pEnemies.cbegin(), pEnemies.cend(), [&pAgent](const EnemyInfo* pEnemy, const EnemyInfo* pOtherEnemy) { return DistanceSquared(pEnemy->Location, pAgent->Position) < DistanceSquared(pOtherEnemy->Location, pAgent->Position); });
}

unsigned char GetIdxOfInventoryItem(const std::unordered_map<unsigned char, ItemInfo*>& pItemsInInventory, eItemType wantedType, IExamInterface* pInterface)
{
	if (wantedType == eItemType::PISTOL)
	{

		std::vector<std::pair<unsigned char, ItemInfo*>> pGunsInInventory{};

		//Find food in item vec, add to temp vec if exist
		auto it = std::find_if(pItemsInInventory.cbegin(), pItemsInInventory.cend(), [wantedType](const std::pair<unsigned char, ItemInfo*>& pGun)
			{
				if (pGun.second)
					return pGun.second->Type == wantedType;
				return false;
			});
		if (it != pItemsInInventory.cend())
			pGunsInInventory.push_back(*it);

		//Add all food items to foodvec
		while (it != pItemsInInventory.cend())
		{
			it = std::find_if(std::next(it), pItemsInInventory.cend(), [wantedType](const std::pair<unsigned char, ItemInfo*>& pGun)
				{
					if (pGun.second)
						return pGun.second->Type == wantedType;
					return false;
				});
			if (it != pItemsInInventory.cend())
				pGunsInInventory.push_back(*it);
		}

		//Go over tempFoodVec, get closest foodItem
		if (!pGunsInInventory.empty())
		{

			auto it = std::min_element(pGunsInInventory.begin(), pGunsInInventory.end(), [&pInterface](const std::pair<unsigned char, ItemInfo*>& pGunA, const std::pair<unsigned char, ItemInfo*>& pGunB)
				{
					return pInterface->Weapon_GetAmmo(*pGunA.second) < pInterface->Weapon_GetAmmo(*pGunB.second); 
				});
			if (it != pGunsInInventory.cend())
				return  it->first;
		}
		return 0;
	}

	auto it = std::find_if(pItemsInInventory.cbegin(), pItemsInInventory.cend(), [wantedType](const std::pair<unsigned char, ItemInfo*>& pItem)
		{
			if(pItem.second) 
				return pItem.second->Type == wantedType; 
			return false;
		});
	if (it != pItemsInInventory.cend())
		return it->first;
	return 0;
}


unsigned char GetIdxOfInventoryItem(const std::unordered_map<unsigned char, ItemInfo*>& pItemsInInventory, eItemType wantedType)
{

	auto it = std::find_if(pItemsInInventory.cbegin(), pItemsInInventory.cend(), [wantedType](const std::pair<unsigned char, ItemInfo*>& pItem)
		{
			if (pItem.second)
				return pItem.second->Type == wantedType;
			return false;
		});
	if (it != pItemsInInventory.cend())
		return it->first;
	return 0;
}


unsigned char GetFirstFreeInventoryIdx(const std::unordered_map<unsigned char, ItemInfo*>& pItemsInInventory)
{
	auto it = std::find_if(pItemsInInventory.cbegin(), pItemsInInventory.cend(), [](const std::pair<unsigned char, ItemInfo*>& pItem) {return pItem.second == nullptr; });
	if (it != pItemsInInventory.cend())
		return it->first;
	return 0;
}



std::vector<unsigned char> GetAllInventoryItemsOfType(const std::unordered_map<unsigned char, ItemInfo*>& pItemsInInventory, eItemType wantedType)
{
	std::vector<unsigned char> pTempItemOfTypeVec{};

	//Find food in item vec, add to temp vec if exist
	auto it = std::find_if(pItemsInInventory.cbegin(), pItemsInInventory.cend(), [wantedType](const std::pair<unsigned char, ItemInfo*>& pItem) 
		{
			if (pItem.second)
				return pItem.second->Type == wantedType;
			return false;
		});
	if (it != pItemsInInventory.cend())
		pTempItemOfTypeVec.push_back(it->first);

	//Add all food items to foodvec
	while (it != pItemsInInventory.cend())
	{
		it = std::find_if(std::next(it), pItemsInInventory.cend(), [wantedType](const std::pair<unsigned char, ItemInfo*>& pItem) 
			{
				if (pItem.second)
					return pItem.second->Type == wantedType;
				return false;
			});
		if (it != pItemsInInventory.cend())
			pTempItemOfTypeVec.push_back(it->first);
	}

	//Go over tempFoodVec, get closest foodItem
	if (!pTempItemOfTypeVec.empty())
		return pTempItemOfTypeVec;
	return std::vector<unsigned char>{};
}
//----------------------------------------------------------------------


std::pair<EntityInfo, ItemInfo*> GetClosestItem(const AgentInfo* pAgent, const std::vector<std::pair<EntityInfo, ItemInfo*>>& pItemsInRange)
{
	//objects in range collection
	//Get closest item
	if (!pItemsInRange.empty())
		return *std::min_element(pItemsInRange.cbegin(), pItemsInRange.cend(), [&pAgent](const std::pair<EntityInfo, ItemInfo*> oA, const std::pair<EntityInfo, ItemInfo*> oB) {return DistanceSquared(pAgent->Position, oA.second->Location) < DistanceSquared(pAgent->Position, oB.second->Location); });

	return std::pair<EntityInfo, ItemInfo*>{};
}

std::pair<EntityInfo, ItemInfo*> GetClosestItemOfType(const AgentInfo* pAgent, const std::vector<std::pair<EntityInfo, ItemInfo*>>& pItemsInRange, const eItemType requiredType)
{
	std::vector<std::pair<EntityInfo, ItemInfo*>> pTempItemOfTypeVec{};

	//Find food in item vec, add to temp vec if exist
	auto it = std::find_if(pItemsInRange.cbegin(), pItemsInRange.cend(), [requiredType](const std::pair<EntityInfo, ItemInfo*> pItem) {return pItem.second->Type == requiredType; });
	if (it != pItemsInRange.cend())
		pTempItemOfTypeVec.push_back(*it);

	//Add all food items to foodvec
	while (it != pItemsInRange.cend())
	{
		it = std::find_if(std::next(it), pItemsInRange.cend(), [requiredType](const std::pair<EntityInfo, ItemInfo*> pItem) {return pItem.second->Type == requiredType; });
		if (it != pItemsInRange.cend())
			pTempItemOfTypeVec.push_back(*it);
	}
	//Go over tempFoodVec, get closest foodItem
	if (!pTempItemOfTypeVec.empty())
		return *std::min_element(pTempItemOfTypeVec.cbegin(), pTempItemOfTypeVec.cend(), [&pAgent](const std::pair<EntityInfo, ItemInfo*> oA, const std::pair<EntityInfo, ItemInfo*> oB) {return DistanceSquared(pAgent->Position, oA.second->Location) < DistanceSquared(pAgent->Position, oB.second->Location); });

	return std::pair<EntityInfo, ItemInfo*>{};
}

//--------------------------------------------------------------
std::unordered_map<int, std::pair<bool, Vector2>> GetAllAvailableWaypoints(const AgentInfo* pAgent, std::unordered_map<int, std::pair<bool, Vector2>> waypoints)
{
	std::unordered_map<int, std::pair<bool, Vector2>> availableWayPoints{};

	//Find food in item vec, add to temp vec if exist
	auto it = std::find_if(waypoints.cbegin(), waypoints.cend(), [](const std::pair<int, std::pair<bool, Vector2>> waypoint)
		{
			return !waypoint.second.first;
		});
	if (it != waypoints.cend())
		availableWayPoints[it->first] = it->second;

	//Add all food items to foodvec
	while (it != waypoints.cend())
	{
		it = std::find_if(std::next(it), waypoints.cend(), [](const std::pair<int, std::pair<bool, Vector2>> waypoint)
			{
				return !waypoint.second.first;
			});

		if (it != waypoints.cend())
			availableWayPoints[it->first] = it->second;

	}

	//Go over tempFoodVec, get closest foodItem
	if (!availableWayPoints.empty())
		return availableWayPoints;
	return std::unordered_map<int, std::pair<bool, Vector2>>{};
}

#endif