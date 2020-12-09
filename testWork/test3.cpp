/* Интересный список */

#include <string>
#include <ostream>
#include <unordered_map>
#include <iostream>
#include <fstream>
#include <vector>
#include <assert.h>

/* Вместо интерфейса I_Serialize(например) для класса  */
std::size_t b_string_write(std::ostream &os, const std::string &value)
{
    const auto pos = os.tellp();
    const uint32_t len = static_cast<std::uint32_t>(value.size());
    os.write(reinterpret_cast<const char *>(&len), sizeof(len));
    if (len > 0)
        os.write(value.data(), len);
    return static_cast<std::size_t>(os.tellp() - pos); // пока не требуется
}

struct ListNode
{
    ListNode *prev;
    ListNode *next;
    ListNode *rand; // NULL or pointer
    std::string data;
};

class List
{
    /*мапа хранит адрес в виде инта и позицию данного элемнта с 0 */
    using index_type = int;

public:
    void Serialize(std::ofstream &stream_to_file);
    void Deserialize(std::ifstream &stream_out_file);

    /* тестовые фукнции для проверки функционала */
    void add_tail(const std::string &data);

    void print_all();

    void simple_init_rand();

private:
    ListNode *m_head = nullptr;
    ListNode *m_tail = nullptr;
    uint32_t m_count = 0;
};

void List::simple_init_rand()
{
    if (m_count != 0)
    {
        ListNode *temp = m_head;

        do
        {
            temp->rand = temp->next;

            temp = temp->next;

        } while (temp != nullptr);
    }
}

void List::add_tail(const std::string &data)
{
    ListNode *temp = new ListNode;

    temp->next = nullptr;
    temp->data = data;
    temp->prev = m_tail;
    temp->rand = nullptr;

    if (m_tail != nullptr)
        m_tail->next = temp;

    if (m_count == 0)
        m_head = m_tail = temp;
    else
        m_tail = temp;

    ++m_count;
}

void List::print_all()
{
    if (m_count != 0)
    {
        ListNode *temp = m_head;

        do
        {
            std::cout << " data = " << temp->data;
            if (temp->rand != nullptr)
                std::cout << " rand data = " << temp->rand->data << std::endl;

            temp = temp->next;

        } while (temp != nullptr);
    }
}

/*  Алгоритм 
    Так как есть ListNode* rand , то буду использовать hashmap, для удобства 
    файл будет выглядеть так (для 3 элементов) : 32AA2BB4CCCC201 // где 2 0 1 это индексы на который указывает rand у текущего элемента,т.е первый элемент указывает на 2(4СССС)
    и указатели по которым я бегаю, они уникальны (не rand которые),поэтому hashmap
*/

void List::Serialize(std::ofstream &stream_to_file)
{
    if (!stream_to_file.is_open())
    {
        std::cout << "failed to open\n";
        return;
    }

    /* Если нету элементов в списке, то сохраняем количетсво = 0 */
    if (m_count == 0)
    {
        stream_to_file.write(reinterpret_cast<const char *>(&m_count), sizeof(m_count));
        return;
    }

    std::unordered_map<std::intptr_t, index_type> map_adress_index;
    index_type index = 0;

    /* Записали количетсво элементов  */
    stream_to_file.write(reinterpret_cast<const char *>(&m_count), sizeof(m_count));

    ListNode *temp = m_head;

    /* записали строки и заполнили u_map*/
    do
    {
        b_string_write(stream_to_file, temp->data);
        map_adress_index.insert(std::make_pair(reinterpret_cast<std::intptr_t>(temp), index++));
        temp = temp->next;
    } while (temp != nullptr);

    temp = m_head;

    do
    {
        if (temp->rand != nullptr)
        {
            auto out = map_adress_index[reinterpret_cast<std::intptr_t>(temp->rand)];
            stream_to_file.write(reinterpret_cast<const char *>(&out), sizeof(index_type));
        }
        else
        {
            index_type error = -1;
            stream_to_file.write(reinterpret_cast<const char *>(&error), sizeof(index_type));
        }

        temp = temp->next;

    } while (temp != nullptr);
}

void List::Deserialize(std::ifstream &stream_out_file)
{
    /* Фукнция очистки списка !!! */
    assert(m_count == 0 && "List not empty");

    std::uint32_t count_temp;

    if (!stream_out_file.is_open() || stream_out_file.peek() == std::ifstream::traits_type::eof())
    {
        std::cout << "not open or empty\n";
        return;
    }

    stream_out_file.read(reinterpret_cast<char *>(&count_temp), sizeof(count_temp));

    if (count_temp <= 0)
    {
        std::cout << "empty list\n";
        return;
    }

    std::uint32_t index_count = 0;

    /* Массив указателей на элементы списка для усатновки round */
    std::vector<ListNode *> v_list;
    v_list.reserve(count_temp);

    /* Первое считывание, заполняем просто список данными без rand  и заполняем вектор указателями на элемнты списка */
    while (index_count < count_temp)
    {
        std::uint32_t size_string_out;

        stream_out_file.read(reinterpret_cast<char *>(&size_string_out), sizeof(size_string_out));

        std::string temp_str;
        temp_str.resize(size_string_out);

        stream_out_file.read(&temp_str[0], size_string_out);

        add_tail(temp_str);

        v_list.push_back(m_tail);

        ++index_count;
    }

    index_count = 0;

    /*Воторое считывание, заполняем */
    while (index_count < count_temp)
    {
        /* Позиция указателя */
        index_type index_out;

        stream_out_file.read(reinterpret_cast<char *>(&index_out), sizeof(index_type));

        if (index_out != -1)
        {
            v_list[index_count]->rand = v_list[index_out];
        }

        ++index_count;
    }
}

int main()
{
    List simple_list;

    //simple_list.add_tail("abc");
    //simple_list.add_tail("efg");
    //simple_list.add_tail("hij");

    //simple_list.simple_init_rand();

    //simple_list.print_all();

    //std::ofstream ofst("txt.b", std::ios::binary);

    //simple_list.Serialize(ofst);

    //ofst.close();

    std::ifstream ifst("txt.b", std::ios::binary);

    simple_list.Deserialize(ifst);

    simple_list.print_all();

    ifst.close();

    return 0;
}