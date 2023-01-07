#ifndef LINKEDLIST_H
#define LINKEDLIST_H

#include <Arduino.h>

class LinkedList
{
    struct Node
    {
        int x1;
        int x2;
        int y1;
        int y2;
        Node *next;
        Node()
        {
            this->x1 = this->x2 = this->y1 = this->y2 = 0;
            this->next = nullptr;
        }
        Node(int x1, int x2, int y1, int y2)
        {
            this->x1 = x1;
            this->x2 = x2;
            this->y1 = y1;
            this->y2 = y2;
            this->next = nullptr;
        }
    };
    Node *head;
    Node *tail;

public:
    LinkedList()
    {
        head = tail = nullptr;
    }
    LinkedList(int x1, int x2, int y1, int y2)
    {
        head = new Node(x1, x2, y1, y2);
        tail = head;
    }

    void insertNode(int x1, int x2, int y1, int y2)
    {
        if (this->head)
        {
            Node *temp = new Node(x1, x2, y1, y2);
            tail->next = temp;
            tail = tail->next;
            temp = nullptr;
        }
        else
        {
            this->tail = this->head = new Node(x1, x2, y1, y2);
        }
    }

    void printNodes()
    {
        Node *temp = this->head;
        while (temp)
        {
            Serial.printf("x1 = %d, x2 = %d, y1 = %d, y2 = %d, Length = %d, Width = %d, Area = %d\n",
                          temp->x1, temp->x2, temp->y1, temp->y2, (temp->x2 - temp->x1), (temp->y2 - temp->y1), (temp->x2 - temp->x1) * (temp->y2 - temp->y1));
            temp = temp->next;
        }
        temp = nullptr;
    }

    bool isNotExplored(int row, int column)
    {
        Node *temp = this->head;
        while (temp)
        {
            if (((row >= temp->x1) && (row <= temp->x2)) &&
                ((column >= temp->y1) && (column <= temp->y2)))
            {
                temp = nullptr;
                return false;
            }
            temp = temp->next;
        }
        temp = nullptr;
        return true;
    }

    void empty()
    {
        if (this->head)
        {
            Node *temp = this->head;
            Node *curr = nullptr;
            while (temp)
            {
                curr = temp;
                temp = temp->next;
                delete curr;
            }
            temp = curr = this->head = this->tail = nullptr;
        }
    }

    ~LinkedList()
    {
        empty();
    }
};

#endif