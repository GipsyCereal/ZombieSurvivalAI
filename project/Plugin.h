#pragma once
#include "IExamPlugin.h"
#include "Exam_HelperStructs.h"
#include "SteeringBehaviors.h"
#include "EGridGraph.h"

class IBaseInterface;
class IExamInterface;

class Plugin :public IExamPlugin
{
public:
	Plugin() {};
	virtual ~Plugin();

	void Initialize(IBaseInterface* pInterface, PluginInfo& info) override;
	void DllInit() override;
	void DllShutdown() override;

	void InitGameDebugParams(GameDebugParams& params) override;
	void Update(float dt) override;

	SteeringPlugin_Output UpdateSteering(float dt) override;
	void Render(float dt) const override;

private:
	//Interface, used to request data from/perform actions with the AI Framework
	IExamInterface* m_pInterface = nullptr;
	vector<HouseInfo> GetHousesInFOV() const;
	vector<EntityInfo> GetEntitiesInFOV() const;

	//------ADDED FUNCTIONS-------
	//Helpers
	void DebugGrid();
	//inits
	void InitBehavior();
	void InitBlackboard();
	void InitGrid(int cellSize);
	//Handlers
	void HandleTimers(const float dt);
	void HandleHouses();
	void HandleEntities();
	void HandleEnemy(const EntityInfo& eInfo, std::vector<EnemyInfo*>& pEnemiesInSight);
	void HandleItem(EntityInfo& eInfo, std::vector<std::pair<EntityInfo, ItemInfo*>>& pItemsInSight);
	void HandlePurgeZone(const EntityInfo& eInfo, std::vector<PurgeZoneInfo*>& pPurgeZones);
	void HandleItemManagement();
	SteeringPlugin_Output HandleSteering(const float dt);
	//----------------------------


	Elite::Vector2 m_Target = {};
	bool m_CanRun = false; //Demo purpose
	bool m_GrabItem = false; //Demo purpose
	bool m_UseItem = false; //Demo purpose
	bool m_RemoveItem = false; //Demo purpose
	float m_AngSpeed = 0.f; //Demo purpose
	//------ADDED VARIABLES-------
	bool m_Run;
	int m_Rows;
	int m_Cols;
	float m_Cooldown;
	//Behavior
	BehaviorTree* m_pBT;
	std::vector<HouseInfo*> m_pExploredHouses{};
	//Containers
	Blackboard* m_pB;
	std::unordered_map<int,std::pair<bool,Vector2>> m_pWayPoints;
	std::unordered_map<unsigned char, ItemInfo*> m_pInventory;
	//Saving enemypointers to look at distance of enemies outside of FOV (if enemy gets too close, shoot if possible, if too far, delete from vector)
	std::vector<EnemyInfo*> m_pEnemies;
	std::unordered_map<std::string, ISteeringBehavior*> m_pBehaviors;
	//----------------------------
};

//ENTRY
//This is the first function that is called by the host program
//The plugin returned by this function is also the plugin used by the host program
extern "C"
{
	__declspec (dllexport) IPluginBase* Register()
	{
		return new Plugin();
	}
}