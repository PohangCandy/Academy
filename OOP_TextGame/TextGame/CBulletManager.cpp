#include "CBulletManager.h"

CBullet* CBulletManager::FindBullet()
{
    for (auto& b : BP)
    {
        if (!b->IsAvailable())
        {
            b->Activate();
            return b;
        }
    }

    return nullptr;
}
