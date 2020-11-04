#include <vector>
#include <math.h>
#include <memory>
#include <algorithm>
#include <iostream>
#include <assert.h>
#include <thread>
#include <chrono>
struct sPos
{
    sPos()
    {
        x = 0;
        y = 0;
    }
    sPos(int aX, int aY)
    {
        x = aX;
        y = aY;
    }

    int x;
    int y;
};

struct sSize
{
    sSize()
    {
        width = 0;
        height = 0;
    }

    sSize(int aW, int aH)
    {
        width = aW;
        height = aH;
    }

    int width;
    int height;
};

struct sRect
{
    sRect() = delete;
    sRect(int x, int y, int w, int h)
    {
        pos.x = x;
        pos.y = y;
        size.width = w;
        size.height = h;
    }
    sPos pos;
    sSize size;

    bool intersects(const sRect &other)
    {
        return !((other.pos.x + other.size.width < pos.x) || (other.pos.y + other.size.height < pos.y) || (other.pos.x > pos.x + size.width) || (other.pos.y > pos.y + size.height));
    }
};

enum class eDirection : unsigned char
{
    UP = 0,
    LEFT = 1,
    RIGHT = 2,
    DOWN = 3
};

class sCar
{
protected:
    sRect rect;
    eDirection dir;
    int speed;

public:
    sCar() = delete;
    sCar(sRect rect, eDirection dir, int speed) : rect(rect), dir(dir), speed(speed) {}

    sRect getCurrentRect()
    {
        return rect;
    }
    eDirection getCurrentDir()
    {
        return dir;
    }
    sRect getFuturePos()
    {
        switch (dir)
        {
        case eDirection::UP:
        {
            return sRect(rect.pos.x, rect.pos.y + speed, rect.size.width, rect.size.height);
        }
        break;
        case eDirection::DOWN:
        {
            return sRect(rect.pos.x, rect.pos.y - speed, rect.size.width, rect.size.height);
        }
        break;
        case eDirection::RIGHT:
        {
            return sRect(rect.pos.x + speed, rect.pos.y, rect.size.width, rect.size.width);
        }
        break;
        case eDirection::LEFT:
        {
            return sRect(rect.pos.x - speed, rect.pos.y, rect.size.width, rect.size.height);
        }
        break;
        default:
            assert(false && "Wrong getFuturePos");
        }
    }

    bool needPassOtherCar(sCar *otherCar)
    {
        bool result = false;
        switch (dir)
        {
        case eDirection::UP:
        {
            auto otherdir = otherCar->dir;
            if (otherdir == eDirection::LEFT)
                result = true;
        }
        break;
        case eDirection::DOWN:
        {
            auto otherdir = otherCar->dir;
            if (otherdir == eDirection::RIGHT)
                result = true;
        }
        break;
        case eDirection::RIGHT:
        {
            auto otherdir = otherCar->dir;
            if (otherdir == eDirection::UP)
                result = true;
        }
        break;
        case eDirection::LEFT:
        {
            auto otherdir = otherCar->dir;
            if (otherdir == eDirection::DOWN)
                result = true;
        }
        break;
        }
        return result;
    }

    virtual void move()
    {
        switch (dir)
        {
        case eDirection::UP:
        {
            rect.pos.y += speed;
        }
        break;
        case eDirection::DOWN:
        {
            rect.pos.y -= speed;
        }
        break;
        case eDirection::RIGHT:
        {
            rect.pos.x += speed;
        }
        break;
        case eDirection::LEFT:
        {
            rect.pos.x -= speed;
        }
        break;
        }
    }

    virtual int getFuel() = 0;
    virtual void refill(int count) = 0;

    virtual ~sCar() = default;
};

class sGasEngine : public virtual sCar
{
protected:
    int fuel;

public:
    sGasEngine(sRect rect, eDirection dir, int speed, int fuel = 0) : sCar(rect, dir, speed), fuel(fuel) {}

    int getFuel() override { return fuel; }
    void refill(int count) override { fuel += count; }

    void move()
    {
        if (fuel > 0)
        {
            fuel--;
            sCar::move();
        }
    }
    virtual ~sGasEngine() = default;
};

struct sElectroCar : public virtual sCar
{
protected:
    int charge;

public:
    sElectroCar(sRect rect, eDirection dir, int speed, int charge = 0) : sCar(rect, dir, speed), charge(charge) {}

    int getFuel() override { return charge; }
    void refill(int count) override { charge += count; }

    void move() override
    {
        if (charge > 0)
        {
            charge--;
            sCar::move();
        }
    }
    virtual ~sElectroCar() = default;
};

class sHybrid : public sGasEngine, public sElectroCar
{
public:
    sHybrid(sRect rect, eDirection dir, int speed, int fuel = 0, int charge = 0) : sGasEngine(rect, dir, speed, fuel), sElectroCar(rect, dir, speed, charge), sCar(rect, dir, speed) {}

    void refill(int count)
    {
        charge += count / 2;
        fuel += count / 2;
    }

    int getFuel() { return charge + fuel; }

    void move() override
    {
        if (charge <= 0 && fuel <= 0)
        {
            return;
        }
        else if (charge > 0 && fuel > 0)
        {
            if (rand() % 2 == 0)
                --charge;
            else
                --fuel;
        }
        else if (charge > 0)
        {
            --charge;
        }
        else if (fuel > 0)
        {
            --fuel;
        }
        sCar::move();
    }
    virtual ~sHybrid() = default;
};

#define SCREEN_WIDTH 1024
#define SCREEN_HEIGHT 768

std::unique_ptr<sCar> spawnCar(sRect spawn_rect, eDirection dir, int speed, int fuel)
{
    auto carType = rand();
    if (carType % 3 == 0)
    {
        return std::unique_ptr<sCar>(new sGasEngine(spawn_rect, dir, speed, fuel));
    }
    else if (carType % 3 == 1)
    {
        return std::unique_ptr<sCar>(new sElectroCar(spawn_rect, dir, speed, fuel));
    }
    else if (carType % 3 == 2)
    {
        return std::unique_ptr<sCar>(new sHybrid(spawn_rect, dir, speed, fuel / 2, fuel / 2));
    }
    else
    {
        assert(false && "invalid carType");
    }
}

/* Важно чтобы машинка зареспавнилась в пределах экрана или нет ? */
std::unique_ptr<sCar> spawnCarFromTop()
{
    auto rect = sRect(SCREEN_WIDTH / 2, SCREEN_HEIGHT, 100, 100);
    auto dir = eDirection::DOWN;
    auto speed = 1;
    auto fuel = 10000;
    return spawnCar(rect, dir, speed, fuel);
}

std::unique_ptr<sCar> spawnCarFromBot()
{
    auto rect = sRect(SCREEN_WIDTH / 2, 0, 100, 100);
    auto dir = eDirection::UP;
    auto speed = 1;
    auto fuel = 10000;

    return spawnCar(rect, dir, speed, fuel);
}

std::unique_ptr<sCar> spawnCarFromLeft()
{
    auto rect = sRect(0, SCREEN_HEIGHT / 2, 100, 100);
    auto dir = eDirection::RIGHT;
    auto speed = 1;
    auto fuel = 10000;
    return spawnCar(rect, dir, speed, fuel);
}

std::unique_ptr<sCar> spawnCarFromRight()
{
    auto rect = sRect(SCREEN_WIDTH, SCREEN_HEIGHT / 2, 100, 100);
    auto dir = eDirection::LEFT;
    auto speed = 1;
    auto fuel = 10000;
    return spawnCar(rect, dir, speed, fuel);
}

std::unique_ptr<sCar> spawnCarDir()
{
    auto spawn_round = rand();
    if (spawn_round % 4 == 0)
        return spawnCarFromRight();
    else if (spawn_round % 4 == 1)
        return spawnCarFromTop();
    else if (spawn_round % 4 == 2)
        return spawnCarFromBot();
    else if (spawn_round % 4 == 3)
        return spawnCarFromLeft();
    else
    {
        assert(false && "invalid spawnCar");
    }
}

/* Задача решается при помощи графов ( академически =) ) , для быстроты я выбрал bruteForce 
    И в задаче не понятно условие выхода из main_loop
*/
void main_loop(std::vector<std::unique_ptr<sCar>> &v_car)
{
    std::vector<std::unique_ptr<sCar> *> vec_4_pool(4);

    while (true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        std::uint8_t count_pass = 0;
        vec_4_pool.clear();

        for (auto &car_1 : v_car)
        {
            bool need_go_current_car = true;
            for (auto &car_2 : v_car)
            {
                if (car_1 == car_2)
                    continue;

                if (car_1->getFuturePos().intersects(car_2->getFuturePos()))
                {
                    if (car_1->needPassOtherCar(car_2.get()))
                    {
                        need_go_current_car = false;
                        ++count_pass;
                        vec_4_pool.push_back(&car_1);
                        std::cout << "need pass otherCar\n";
                        continue;
                    }
                }
            }

            if (need_go_current_car)
            {
                car_1->move();
                std::cout << static_cast<int>(car_1->getCurrentDir()) << std::endl;
                std::cout << car_1->getCurrentRect().pos.x << " " << car_1->getCurrentRect().pos.y << " " << car_1->getCurrentRect().size.height << " " << car_1->getCurrentRect().size.width << std::endl;
            }

            if (count_pass == 4)
            {
                std::this_thread::sleep_for(std::chrono::seconds(1));
                std::cout << "STOP\n";

                auto iter_small = std::min_element(vec_4_pool.begin(), vec_4_pool.end(),
                                                   [](std::unique_ptr<sCar> *elem1, std::unique_ptr<sCar> *elem2) {
                                                       return elem1->get()->getCurrentRect().pos.x < elem2->get()->getCurrentRect().pos.x;
                                                   });
                (*iter_small)->get()->move();
                std::cout << static_cast<int>((*iter_small)->get()->getCurrentDir()) << std::endl;
                std::cout << (*iter_small)->get()->getCurrentRect().pos.x << " " << (*iter_small)->get()->getCurrentRect().pos.y << " " << (*iter_small)->get()->getCurrentRect().size.height << " " << (*iter_small)->get()->getCurrentRect().size.width << std::endl;
            }
        }
    }
}

void safeCarSpammer(std::vector<std::unique_ptr<sCar>> &v_car)
{
    auto newCar = spawnCarDir();

    auto lamda = [newCar = newCar.get()](auto &car) {
        return car->getCurrentRect().intersects(newCar->getCurrentRect());
    };

    if (std::find_if(v_car.begin(), v_car.end(), lamda) == v_car.end())
    {
        v_car.push_back(std::move(newCar));
    }
}

int main(int argc, char **argv)
{
    const int initialCarsCount = 10;
    std::vector<std::unique_ptr<sCar>> v_car;
    v_car.reserve(initialCarsCount);

    for (int i = 0; i < initialCarsCount; ++i)
    {
        safeCarSpammer(v_car);
    }

    main_loop(v_car);

    return 0;
}
