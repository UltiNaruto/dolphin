#pragma once

#include "Core/PrimeHack/PrimeMod.h"

namespace prime {
class SpringballButton : public PrimeMod {
public:
  void run_mod(Game game, Region region) override;
  bool init_mod(Game game, Region region) override;
  void on_state_change(ModState old_state) override {}

private:
  void springball_code_wii(u32 start_point);
  void springball_code_gcn(u32 start_point, u32 return_point, u32 player_state_offset);
  void springball_check_wii(u32 ball_address, u32 movement_address);
  void springball_check_gcn(u32 spring_ball_trigger_address, u32 x_pos_address, u32 ball_address, u32 movement_address);
};
}
