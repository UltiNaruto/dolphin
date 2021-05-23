#include "Core/PrimeHack/Mods/SpringballButton.h"

#include "Core/PrimeHack/PrimeUtils.h"

#define BL(dst, src) 0x48000001 | (((dst) - (src)) & 0x3ffffff)
#define LIS_ORI(reg, addr) std::pair<u32, u32>(0x3c000000 | (reg << 21) | ((addr) & 0xFFFF0000) >> 16, 0x60000000 | (reg << 21) | (reg << 16) | ((addr) & 0xFFFF))

namespace prime {

void SpringballButton::run_mod(Game game, Region region) {
  u8 version = read8(0x80000007);
  LOOKUP_DYN(player);
  if (player == 0) {
    return;
  }
  switch (game) {
  case Game::PRIME_1:
    springball_check_wii(player + 0x2f4, player + 0x25c);
    break;
  case Game::PRIME_1_GCN:
    if (region == Region::NTSC_U) {
      if (version == 0) {
        springball_check_gcn(0x805afd18, player + 0x40, player + 0x2f4, player + 0x258);
      }
    } else if (region == Region::NTSC_J) {
      springball_check_gcn(0x80599118, player + 0x40, player + 0x304, player + 0x268);
    } else if (region == Region::PAL) {
      springball_check_gcn(0x80471d18, player + 0x40, player + 0x304, player + 0x268);
    }
    break;
  case Game::PRIME_2:
    springball_check_wii(player + 0x374, player + 0x2c4);
    break;
  case Game::PRIME_2_GCN:
    if (region == Region::NTSC_J) {
      springball_check_gcn(0x80423000, player + 0x30, player + 0x38c, player + 0x2d0);
    } else { // region == Region::NTSC_U || region == Region::PAL
      springball_check_gcn(0x80421000, player + 0x30, player + 0x38c, player + 0x2d0);
    }
    break;
  case Game::PRIME_3:
  case Game::PRIME_3_STANDALONE:
    springball_check_wii(player + 0x358, player + 0x29c);
    break;
  default:
    break;
  }
}

bool SpringballButton::init_mod(Game game, Region region) {
  LOOKUP(compute_ball_movement);

  u8 version = read8(0x80000007);

  if (game < Game::PRIME_1_GCN) {
    prime::GetVariableManager()->register_variable("springball_trigger");
  }

  switch (game) {
  case Game::PRIME_1:
    if (region == Region::NTSC_U) {
      springball_code_wii(0x801476d0);
    } else if (region == Region::PAL) {
      springball_code_wii(0x80147820);
    } else if (region == Region::NTSC_J) {
      springball_code_wii(0x80147cd0);
    }
    break;
  case Game::PRIME_1_GCN:
    if (region == Region::NTSC_U) {
      if (version == 0) {
        // 0x805afd18 => springball_trigger
        // 0x805afd1c => player_x
        // 0x805afd20 => player_y
        // 0x805afd24 => player_z
        springball_code_gcn(0x805afd28, compute_ball_movement + 0x2c, 0x8b8);
      }
    } else if (region == Region::NTSC_J) {
      springball_code_gcn(0x80599128, compute_ball_movement + 0x2c, 0x8b8);
    } else if (region == Region::PAL) {
      springball_code_gcn(0x80471d28, compute_ball_movement + 0x2c, 0x8b8);
    }
    // MP2 0x80421000
    break;
  case Game::PRIME_2:
    if (region == Region::NTSC_U) {
      springball_code_wii(0x8010bd98);
    } else if (region == Region::PAL) {
      springball_code_wii(0x8010d440);
    } else if (region == Region::NTSC_J) {
      springball_code_wii(0x8010b368);
    }
    break;
  case Game::PRIME_2_GCN:
    if (region == Region::NTSC_J) {
      springball_code_gcn(0x80423010, compute_ball_movement + 0x68, 0x150c);
    } else { // region == Region::NTSC_U || region == Region::PAL
      springball_code_gcn(0x80421010, compute_ball_movement + 0x68, 0x150c);
    }
    break;
  case Game::PRIME_3:
    if (region == Region::NTSC_U) {
      springball_code_wii(0x801077d4);
    } else if (region == Region::PAL) {
      springball_code_wii(0x80107120);
    }
    break;
  case Game::PRIME_3_STANDALONE:
    if (region == Region::NTSC_U) {
      springball_code_wii(0x8010c984);
    } else if (region == Region::PAL) {
      springball_code_wii(0x8010ced4);
    } else if (region == Region::NTSC_J) {
      springball_code_wii(0x8010d49c);
    }
    break;
  default:
    break;
  }
  return true;
}

void SpringballButton::springball_code_wii(u32 start_point) {
  u32 lis, ori;
  std::tie<u32, u32>(lis, ori) = prime::GetVariableManager()->make_lis_ori(4, "springball_trigger");

  add_code_change(start_point + 0x00, lis);  // lis r4, springball_trigger_upper
  add_code_change(start_point + 0x04, ori);  // ori r4, r4, springball_trigger_lower
  add_code_change(start_point + 0x08, 0x88640000);  // lbz r3, 0(r4)
  add_code_change(start_point + 0x0c, 0x38A00000);  // li r5, 0
  add_code_change(start_point + 0x10, 0x98A40000);  // stb r5, 0(r4)
  add_code_change(start_point + 0x14, 0x2C030000);  // cmpwi r3, 0
}

void SpringballButton::springball_code_gcn(u32 start_point, u32 return_point, u32 player_state_offset)
{
  LOOKUP(compute_boostball_movement);
  LOOKUP(compute_ball_movement);
  LOOKUP(is_ball_movement_allowed);
  LOOKUP(player_bomb_jump);
  LOOKUP(player_has_powerup);

  u32 lis, ori;

  // Stack init
  add_code_change(start_point + 0x00, 0x9421ffe0); // stwu r1, -0x20(sp)
  add_code_change(start_point + 0x04, 0x7c0802a6); // mflr r0
  add_code_change(start_point + 0x08, 0x90010024); // stw r0, 0x24(sp)
  add_code_change(start_point + 0x0c, 0x93e1001c); // stw r31, 0x1c(sp)
  add_code_change(start_point + 0x10, 0x93c10018); // stw r30, 0x18(sp)
  add_code_change(start_point + 0x14, 0x7cbe2b78); // or r30, r5, r5
  add_code_change(start_point + 0x18, 0x93a10014); // stw r29, 0x14(sp)
  add_code_change(start_point + 0x1c, 0x7c9d2378); // or r29, r4, r4
  add_code_change(start_point + 0x20, 0x93810010); // stw r28, 0x10(sp)
  add_code_change(start_point + 0x24, 0x7c7c1b78); // or r28, r3, r3

  // Function body
  add_code_change(start_point + 0x28, BL(is_ball_movement_allowed, start_point + 0x28)); // bl CMorphBall:IsMovementAllowed
  add_code_change(start_point + 0x2c, 0x2c030000); // cmpwi r3, 0
  add_code_change(start_point + 0x30, 0x4182004c); // beq CMorphBall::ComputeSpringBallMovement + 0x7c
  add_code_change(start_point + 0x34, 0x807e0000 | player_state_offset); // lwz r3, player_state_offset(r30)
  add_code_change(start_point + 0x38, player_state_offset == 0x150c ? 0x60000000 : 0x80630000); // nop or lwz r3, 0(r3)
  add_code_change(start_point + 0x3c, 0x38800000 | (player_state_offset == 0x150c ? 18 : 6)); // li r4, EItemType::Bombs
  add_code_change(start_point + 0x40, BL(player_has_powerup, start_point + 0x40)); // bl CPlayerState::HasPowerUp
  add_code_change(start_point + 0x44, 0x2c030000); // cmpwi r3, 0
  add_code_change(start_point + 0x48, 0x41820034); // beq CMorphBall::ComputeSpringBallMovement + 0x7c

  std::tie<u32, u32>(lis, ori) = LIS_ORI(4, start_point - 0x10);

  add_code_change(start_point + 0x4c, lis);        // lis r4, springball_trigger_upper
  add_code_change(start_point + 0x50, ori);        // ori r4, r4, springball_trigger_lower
  add_code_change(start_point + 0x54, 0x88640000); // lbz r3, 0(r4)
  add_code_change(start_point + 0x58, 0x38A00000); // li r5, 0
  add_code_change(start_point + 0x5c, 0x98A40000); // stb r5, 0(r4)
  add_code_change(start_point + 0x60, 0x2C030000); // cmpwi r3, 0
  add_code_change(start_point + 0x64, 0x41820018); // beq CMorphBall::ComputeSpringBallMovement + 0x7c
  add_code_change(start_point + 0x68, 0x807c0000); // lwz r3, 0(r28)

  std::tie<u32, u32>(lis, ori) = LIS_ORI(4, start_point - 0x0c);

  add_code_change(start_point + 0x6c, lis); // lis r4, player_x_upper
  add_code_change(start_point + 0x70, ori); // ori r4, player_x_lower
  add_code_change(start_point + 0x74, 0x7fc5f378); // or r5, r30, r30
  add_code_change(start_point + 0x78, BL(player_bomb_jump, start_point + 0x78)); // bl CPlayer::BombJump
  add_code_change(start_point + 0x7c, 0x7f83e378); // or r3, r28, r28
  add_code_change(start_point + 0x80, 0x7fa4eb78); // or r4, r29, r29
  add_code_change(start_point + 0x84, 0x7fc5f378); // or r5, r30, r30
  add_code_change(start_point + 0x88, BL(compute_boostball_movement, start_point + 0x88)); // bl CMorphBall::ComputeBoostBallMovement

  // Stack uninit
  add_code_change(start_point + 0x8c, 0x80010024); // lwz r0, 0x24(sp)
  add_code_change(start_point + 0x90, 0x83e1001c); // lwz r31, 0x1c(sp)
  add_code_change(start_point + 0x94, 0x83c10018); // lwz r30, 0x18(sp)
  add_code_change(start_point + 0x98, 0x83a10014); // lwz r29, 0x14(sp)
  add_code_change(start_point + 0x9c, 0x83810010); // lwz r28, 0x10(sp)
  add_code_change(start_point + 0xa0, 0x7c0803a6); // mtlr r0
  add_code_change(start_point + 0xa4, 0x38210020); // addi sp, sp, 0x20

  // End of CMorphBall::ComputeSpringBallMovement function
  add_code_change(start_point + 0xa8, 0x4e800020); // blr

  // Replace CMorphBall::ComputeBoostBallMovement by CMorphBall::ComputeSpringBallMovement
  add_code_change(return_point, BL(start_point, return_point)); // bl CMorphBall::ComputeSpringBallMovement
}

void SpringballButton::springball_check_wii(u32 ball_address, u32 movement_address) {
  if (CheckSpringBallCtl()) {
    const u32 ball_state = read32(ball_address);
    const u32 movement_state = read32(movement_address);

    if ((ball_state == 1 || ball_state == 2) && movement_state == 0) { 
      prime::GetVariableManager()->set_variable("springball_trigger", u8{ 1 });
    }
  }
}

void SpringballButton::springball_check_gcn(u32 spring_ball_trigger_address, u32 x_pos_address, u32 ball_address, u32 movement_address) {
  // update position for the bomb jump
  writef32(readf32(x_pos_address), spring_ball_trigger_address + 0x4); // x
  writef32(readf32(x_pos_address + 0x10), spring_ball_trigger_address + 0x8); // y
  writef32(readf32(x_pos_address + 0x20), spring_ball_trigger_address + 0xc); // z

  if (CheckSpringBallCtl()) {
    const u32 ball_state = read32(ball_address);
    const u32 movement_state = read32(movement_address);

    if ((ball_state == 1 || ball_state == 2) && movement_state == 0) {
      write8(1, spring_ball_trigger_address);
    }
  } else {
    write32(0, spring_ball_trigger_address);
  }
}

}
