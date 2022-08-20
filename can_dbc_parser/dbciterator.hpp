/*
 * dbctree.hpp
 *
 *  Created on: 04.10.2013
 *      Author: downtimes
 */

#ifndef DBCTREE_HPP_
#define DBCTREE_HPP_

#include <vector>
//#include <iosfwd>
#include "message.hpp"

/**
 * This is the Top class of the dbclib and the interface to the user.
 * It enables its user to iterate over the Messages of a DBC-File
 */

class DBCIterator {

	typedef std::vector<Message> messages_t;
	//This list contains all the messages which got parsed from the DBC-File
	messages_t messageList;

public:
	typedef messages_t::const_iterator const_iterator;

	//Constructors taking either a File or a Stream of a DBC-File
	DBCIterator() {};
	DBCIterator(const std::string& filePath);
	DBCIterator(std::istream& stream);

	bool open(const std::string& filePath);
	/*
	 * Functionality to access the Messages parsed from the File
	 * either via the iterators provided by begin() and end() or by
	 * random access operator[]
	 */
	const_iterator begin() const { return messageList.begin(); }
	const_iterator end() const { return messageList.end(); }
	messages_t::const_reference operator[](std::size_t elem) const {
		return messageList[elem];
	}

	int count() { return messageList.size(); }
	int signal_count( std::size_t elem ) { return messageList[elem].signal_count(); }

	//Message class���� set_cycle_time �Լ��� �����ϰ�
	//���ο��� m_dbc[i].set_cycle_time( pCycle[j] ); �� ���� ����ϴ� ������ ���� ������ �߻��Ѵ�.
	//error C2662: 'Message::set_list_index' : cannot convert 'this' pointer from 'const Message' to 'Message &'
	//�׷��� ���⿡ �Լ��� ���� �����ϰ� ���⿡�� �ش� messageList �׸��� cycle time�� �������ش�.
	void set_cycle_time( std::size_t elem, std::size_t cycle )
	{
		messageList[elem].set_cycle_time( cycle );
	}

	void set_list_index( std::size_t elem, uint32_t dwIndex )
	{
		messageList[elem].set_list_index( dwIndex );
	}

private:
	void init(std::istream& stream);
};




#endif /* DBCTREE_HPP_ */
