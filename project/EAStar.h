#pragma once

namespace Elite
{
	template <class T_NodeType, class T_ConnectionType>
	class AStar
	{
	public:
		AStar(IGraph<T_NodeType, T_ConnectionType>* pGraph, Heuristic hFunction);

		// stores the optimal connection to a node and its total costs related to the start and end node of the path
		struct NodeRecord
		{
			T_NodeType* pNode = nullptr;
			T_ConnectionType* pConnection = nullptr;
			float costSoFar = 0.f; // accumulated g-costs of all the connections leading up to this one
			float estimatedTotalCost = 0.f; // f-cost (= costSoFar + h-cost)

			bool operator==(const NodeRecord& other) const
			{
				return pNode == other.pNode
					&& pConnection == other.pConnection
					&& costSoFar == other.costSoFar
					&& estimatedTotalCost == other.estimatedTotalCost;
			};

			bool operator<(const NodeRecord& other) const
			{
				return estimatedTotalCost < other.estimatedTotalCost;
			};
		};

		std::vector<T_NodeType*> FindPath(T_NodeType* pStartNode, T_NodeType* pDestinationNode);
		void DebugGraph();

	private:
		float GetHeuristicCost(T_NodeType* pStartNode, T_NodeType* pEndNode) const;

		IGraph<T_NodeType, T_ConnectionType>* m_pGraph;
		Heuristic m_HeuristicFunction;
	};

	template <class T_NodeType, class T_ConnectionType>
	AStar<T_NodeType, T_ConnectionType>::AStar(IGraph<T_NodeType, T_ConnectionType>* pGraph, Heuristic hFunction)
		: m_pGraph(pGraph)
		, m_HeuristicFunction(hFunction)
	{
	}

	template <class T_NodeType, class T_ConnectionType>
	std::vector<T_NodeType*> AStar<T_NodeType, T_ConnectionType>::FindPath(T_NodeType* pStartNode, T_NodeType* pGoalNode)
	{
		//ASTAR
		vector<T_NodeType*> path;
		vector<NodeRecord> openList;
		vector<NodeRecord> closedList;
		NodeRecord currentRecord{};
		NodeRecord startRecord;
		startRecord.pNode = pStartNode;
		startRecord.pConnection = nullptr;
		startRecord.estimatedTotalCost = GetHeuristicCost(pStartNode,pGoalNode);

		openList.push_back(startRecord);
		while (!openList.empty())
		{
		
			//Assign shortest path to currRec
			currentRecord = *std::min_element(openList.begin(), openList.end(), [](const NodeRecord& a, const NodeRecord& b) {return a.estimatedTotalCost < b.estimatedTotalCost; });
			
			//Does currRec lead to endNode if so break
			if (currentRecord.pConnection && currentRecord.pConnection->GetTo() == pGoalNode->GetIndex())
				break;
			//if not, get all connections from the connections end node
		
				Elite::Vector2 possDiff{ m_pGraph->GetNodePos(currentRecord.pNode->GetIndex()) - m_pGraph->GetNodePos(pStartNode->GetIndex()) };
			for (auto con : m_pGraph->GetNodeConnections(currentRecord.pNode->GetIndex()))
			{
				bool addNewRecord{ false };
				//Check if node at the end of the connection is already in the closed list,If not then we havent checked if its a possible better option
				auto pConnectionEndNode{m_pGraph->GetNode(con->GetTo())};
				//is there an existing connection to this node?
				auto equalNodeIt{ std::find_if(closedList.begin(), closedList.end(), [&pConnectionEndNode](const NodeRecord& a) {return a.pNode == pConnectionEndNode; }) };
				float gCost{ currentRecord.costSoFar + con->GetCost()};

				if (equalNodeIt!= closedList.end())
				{
					const float gCostClosedList{ equalNodeIt->costSoFar };
					//is the now checking connection is shorter then the existing 
					if (gCost < gCostClosedList)
					{
						//if so erase the connection in the list so u can replace it with the improved one later
						closedList.erase(equalNodeIt);
						addNewRecord = true;
					}
						
					
				}
				//openList
				else
				{
					//is there an existing connection to this node?
					auto equalNodeIt{ std::find_if(openList.begin(), openList.end(), [&pConnectionEndNode](const NodeRecord& a) {return a.pNode == pConnectionEndNode; }) };
					//if so
					if (equalNodeIt != openList.end())
					{
						const float gCostOpenList{ equalNodeIt->costSoFar };
						//is the now checking connection is shorter then the existing 
						if (gCost < gCostOpenList)
						{
							//if so erase the connection in the list so u can replace it with the improved one later
							openList.erase(equalNodeIt);
							addNewRecord = true;

						}
					}
					else
					{ 
						//There is no connection to the end node in the lists thus add the connection and node
						addNewRecord = true;
					}
				}
				if (addNewRecord)
				{
					NodeRecord newRecord{m_pGraph->GetNode(pConnectionEndNode->GetIndex()),con,gCost,GetHeuristicCost(pConnectionEndNode,pGoalNode) + gCost};
					openList.push_back(newRecord);

				}
			}

			for (auto it = openList.begin(); it != openList.end(); it++)
			{
				if (*it == currentRecord)
				{
					openList.erase(it);
					closedList.push_back(currentRecord);
					break;
				}
			}
		
	
		}

			while (currentRecord.pNode != pStartNode)
			{
				path.push_back(currentRecord.pNode);
				auto foundRecord{ std::find_if(closedList.begin(), closedList.end(), [&currentRecord, this](const NodeRecord& a) {return currentRecord.pConnection->GetFrom() == a.pNode->GetIndex(); }) };
				if(foundRecord!=closedList.end())
					currentRecord = *foundRecord;
			}
			path.push_back(pStartNode);
			std::reverse(path.begin(), path.end());


		return path;
	}

	template<class T_NodeType, class T_ConnectionType>
	inline void AStar<T_NodeType, T_ConnectionType>::DebugGraph()
	{
#if defined(_DEBUG)
		for (size_t i = 0; i < 10; i++)
		{
			for (size_t j = 0; j < 20; j++)
			{
				DEBUGRENDERER2D->DrawString(Elite::Vector2(15 * j, 15 * (i+1)), std::to_string(m_pGraph->GetAllNodes()[i * 20 + j]->GetIndex()).c_str());

			}
		}
#endif
	}

	template <class T_NodeType, class T_ConnectionType>
	float Elite::AStar<T_NodeType, T_ConnectionType>::GetHeuristicCost(T_NodeType* pStartNode, T_NodeType* pEndNode) const
	{
		Vector2 toDestination = m_pGraph->GetNodePos(pEndNode) - m_pGraph->GetNodePos(pStartNode);
		return m_HeuristicFunction(abs(toDestination.x), abs(toDestination.y));
	}

}