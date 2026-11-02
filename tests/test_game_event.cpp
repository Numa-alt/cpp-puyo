#include <cassert>
#include "GameEvent.h"

int main()
{
    GameEvent event(
        GameEventType::Chain,
        5,
        11,
        10
        );

    assert(event.GetType() == GameEventType::Chain);
    assert(event.GetX() == 5);
    assert(event.GetY() == 11);
    assert(event.GetValue() == 10);
    return 0;
}