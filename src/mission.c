/* mission.c — mission_update / mission_init 은 game.c 내부 로직으로 통합됨.
 * 이 파일은 Mission 배열 정의만 유지. */
#include "mission.h"

Mission missions[MISSION_COUNT];

void mission_init(Mission *m, int index)
{
    m->complete = 0;
    m->type     = MISSION_DIP_PATTERN;
    m->target   = 0;
    (void)index;
}

int mission_update(GameCtx *ctx, Mission *m)
{
    (void)ctx; (void)m;
    return 0; /* 실제 로직은 game.c의 mission_step()에 있음 */
}
