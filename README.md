# ZombieSurvivalAI
A C++ project focussed on making an NPC survive as long as possible in a Zombie hoarded environment by implementing AI Gameplay programming in a custom C++ framework with limited world data.

## NPC A.I.
To make my NPC agent flexible in changing behaviour conditionally, I implemented a Behaviour Tree combined with Steering Behaviours.
The Conditionals and States of the Behaviour Tree were iterated through providing relevant data through a Blackboard pattern.

### Some examples of Conditionals
```cpp
bool IsBitten(Elite::Blackboard* pB)
{
	AgentInfo* pAgent{ nullptr };
	bool turning{};
	const bool dataAvailable{ pB->GetData("Agent", pAgent) && pB->GetData("Turning", turning) };
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

bool IsAimingAtEnemy(Elite::Blackboard* pB)
{
	AgentInfo* pAgent{ nullptr };
	std::vector<EnemyInfo*> pEnemies{};
	const bool dataAvailable{ pB->GetData("Agent", pAgent) && pB->GetData("Enemies", pEnemies) };
	if (!dataAvailable || !pAgent || pEnemies.empty())
		return false;
	EnemyInfo* pDangerousEnemy{ GetEnemyByPriority(pAgent, pEnemies) };
	//Endpoint of the shooting line trace
	const Vector2 A{ pAgent->Position };
	const Vector2 B{ pAgent->Position.x + pAgent->FOV_Range * cosf(pAgent->Orientation), pAgent->Position.y + pAgent->FOV_Range * sinf(pAgent->Orientation) };
	if( DistanceSquared(B, pDangerousEnemy->Location) < 1)
		return true;
	return IsLineSphereIntersection(pAgent, pDangerousEnemy);
}
```
The IsLineSphereIntersection method:

```cpp
bool IsLineSphereIntersection(const AgentInfo* pAgent, const EnemyInfo* pEnemy)
{
	const Vector2 rayO{ pAgent->Position };
	const Vector2 rayD{ cosf(pAgent->Orientation -  static_cast<float>(M_PI) / 2), sinf(pAgent->Orientation - static_cast<float>(M_PI) / 2)};
	const Vector2 center{pEnemy->Location};
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
```

An example of a behaviour state (state that triggers when a sequence/selector returns true)

```cpp
BehaviorState UseItemOfType(Elite::Blackboard* pBlackboard)
{
	AgentInfo* pAgent{ nullptr };
	eItemType wantedType{};
	std::unordered_map<unsigned char, ItemInfo*>* pInventory{};
	IExamInterface* pInterface{ nullptr };


	bool dataAvailable{ pBlackboard->GetData("Agent", pAgent)
		&& pBlackboard->GetData("pInterface", pInterface)
		&& pBlackboard->GetData("Inventory", pInventory)
		&& pBlackboard->GetData("WantedType", wantedType) };
	if (!dataAvailable || !pAgent || !pInterface)
		return Failure;
	//Gets indices of items in inventory
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
```

