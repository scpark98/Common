template <class T>
class TemplateList
{
private:
	int m_nNumNode;
		
	struct Node
	{
		T		data;
		Node	*pHeadLink;
		Node	*pTailLink;
	};
		
	Node*	m_pHeadPos;
	Node*	m_pTailPos;
	Node*	m_pCurrent;		//임의의 위치에 있는 리스트의 노드들에 접근할 수 있도록 한다.

public:
	//생성자
	TemplateList()
	{			
		m_nNumNode		= 0;
		m_pHeadPos		= NULL;
		m_pTailPos		= NULL;
		m_pCurrent		= NULL;
	}
		
	//소멸자
	~TemplateList()
	{
		ReleaseAll();			
	}
		
	//리스트의 끝에 노드 추가
	void AddNodeAtTail( T pItem )
	{
		Node *pNode		= NULL;
			
		pNode			= new struct Node;
		pNode->data		= pItem;
		pNode->pHeadLink = NULL;
		pNode->pTailLink = NULL;
			
		if ( m_nNumNode == 0 )
		{
			m_nNumNode	= 1;
			m_pHeadPos	= pNode;
			m_pTailPos	= pNode;
			m_pCurrent	= pNode;
		}
		else
		{
			m_pTailPos->pTailLink	= pNode;
			pNode->pHeadLink		= m_pTailPos;
			m_pTailPos				= m_pCurrent = pNode;
			m_nNumNode++;
		}
	}

	//현재 포인터가 위치한 노드를 삭제하고 포인터는 삭제된 노드의 전 단계 노드로 옮긴다.
	BOOL DeleteCurrentNode()	
	{
		Node *pDeleteNode;
			
		//데이터가 없다면 리턴
		if ( m_pCurrent == NULL )
			return FALSE;
		
		pDeleteNode = m_pCurrent;

		//노드가 1개인 경우
		if ( pDeleteNode->pHeadLink == NULL && pDeleteNode->pTailLink == NULL )
		{
			m_pHeadPos = NULL;
			m_pTailPos = NULL;
			m_pCurrent = NULL;
		}
		//해드노드를 삭제하는 경우 다음 노드가 현재 노드가 된다.
		else if ( pDeleteNode->pHeadLink == NULL )
		{
			pDeleteNode->pTailLink->pHeadLink = NULL;
			m_pHeadPos = m_pCurrent = pDeleteNode->pTailLink;
		}
		//테일링크를 삭제하는 경우는 바로 전 노드가 현재 노드가 된다.
		else if ( pDeleteNode->pTailLink == NULL )
		{
			pDeleteNode->pHeadLink->pTailLink = NULL;
			m_pTailPos = m_pCurrent = pDeleteNode->pHeadLink;
		}
		else
		{
			(pDeleteNode->pHeadLink)->pTailLink = m_pCurrent = pDeleteNode->pTailLink;
			(pDeleteNode->pTailLink)->pHeadLink = pDeleteNode->pHeadLink;
		}
			
		m_nNumNode--;
		delete pDeleteNode;

		return TRUE;
	}

	void ModifyCurrentData( T data )
	{
		m_pCurrent->data = data;
	}
		
	//다음 노드로 현재 포인터 이동
	BOOL GotoNextNode()
	{
		if ( m_pCurrent->pTailLink != NULL )
		{
			m_pCurrent = m_pCurrent->pTailLink;
			return TRUE;
		}
		else
		{
			return FALSE;
		}
			
	}

	//이전 노드로 현재 포인터 이동
	BOOL GotoPrevNode()
	{
		if ( m_pCurrent->pHeadLink != NULL )
		{
			m_pCurrent = m_pCurrent->pHeadLink;
			return TRUE;
		}
		else
		{
			return FALSE;
		}
	}

	//맨 처음 노드로 이동
	BOOL GotoHeadNode(void)
	{
		if ( m_pHeadPos != NULL )
		{
			m_pCurrent = m_pHeadPos;
			return TRUE;
		}
		else
		{
			return FALSE;
		}
	}

	//맨 끝 노드로 이동
	BOOL GotoTailNode()
	{
		if ( m_pTailPos != NULL )
		{
			m_pCurrent = m_pTailPos;
			return TRUE;
		}
		else
		{
			return FALSE;
		}
	}

	BOOL IsTailNode()
	{
		if ( m_pCurrent == NULL )
			return TRUE;

		return ( m_pCurrent->pTailLink == NULL );
	}

	BOOL IsHeadNode()
	{
		if ( m_pCurrent == NULL )
			return TRUE;

		return ( m_pCurrent->pHeadLink == NULL );
	}
		
	//현재 포인터가 위치한 지점의 데이터를 얻어옴
	BOOL GetCurrentData( T& data )
	{
		if ( m_nNumNode <= 0 )
			return FALSE;

		if ( m_pCurrent == NULL )
			return FALSE;

		data = m_pCurrent->data;
		return TRUE;
	}

	//전체 노드의 갯수
	int GetCount()
	{
		return m_nNumNode;
	}

	//할당된 메모리 반환
	void ReleaseAll()
	{
		Node *pNode0, *pNode1;

		pNode0 = m_pHeadPos;
		while ( pNode0 )
		{
			pNode1 = pNode0->pTailLink;
			delete pNode0;
			pNode0 = pNode1;
		}
			
		m_pHeadPos = NULL;
		m_pTailPos = NULL;
		m_pCurrent = NULL;
		m_nNumNode = 0;
	}
};
