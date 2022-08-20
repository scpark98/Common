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
	Node*	m_pCurrent;		//������ ��ġ�� �ִ� ����Ʈ�� ���鿡 ������ �� �ֵ��� �Ѵ�.

public:
	//������
	TemplateList()
	{			
		m_nNumNode		= 0;
		m_pHeadPos		= NULL;
		m_pTailPos		= NULL;
		m_pCurrent		= NULL;
	}
		
	//�Ҹ���
	~TemplateList()
	{
		ReleaseAll();			
	}
		
	//����Ʈ�� ���� ��� �߰�
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

	//���� �����Ͱ� ��ġ�� ��带 �����ϰ� �����ʹ� ������ ����� �� �ܰ� ���� �ű��.
	BOOL DeleteCurrentNode()	
	{
		Node *pDeleteNode;
			
		//�����Ͱ� ���ٸ� ����
		if ( m_pCurrent == NULL )
			return FALSE;
		
		pDeleteNode = m_pCurrent;

		//��尡 1���� ���
		if ( pDeleteNode->pHeadLink == NULL && pDeleteNode->pTailLink == NULL )
		{
			m_pHeadPos = NULL;
			m_pTailPos = NULL;
			m_pCurrent = NULL;
		}
		//�ص��带 �����ϴ� ��� ���� ��尡 ���� ��尡 �ȴ�.
		else if ( pDeleteNode->pHeadLink == NULL )
		{
			pDeleteNode->pTailLink->pHeadLink = NULL;
			m_pHeadPos = m_pCurrent = pDeleteNode->pTailLink;
		}
		//���ϸ�ũ�� �����ϴ� ���� �ٷ� �� ��尡 ���� ��尡 �ȴ�.
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
		
	//���� ���� ���� ������ �̵�
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

	//���� ���� ���� ������ �̵�
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

	//�� ó�� ���� �̵�
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

	//�� �� ���� �̵�
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
		
	//���� �����Ͱ� ��ġ�� ������ �����͸� ����
	BOOL GetCurrentData( T& data )
	{
		if ( m_nNumNode <= 0 )
			return FALSE;

		if ( m_pCurrent == NULL )
			return FALSE;

		data = m_pCurrent->data;
		return TRUE;
	}

	//��ü ����� ����
	int GetCount()
	{
		return m_nNumNode;
	}

	//�Ҵ�� �޸� ��ȯ
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
