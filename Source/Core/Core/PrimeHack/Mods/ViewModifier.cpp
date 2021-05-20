#include "Core/PrimeHack/Mods/ViewModifier.h"

#include "Core/PrimeHack/PrimeUtils.h"

namespace prime {
void ViewModifier::run_mod(Game game, Region region) {
  switch (game) {
  case Game::PRIME_1:
    run_mod_mp1();
    break;
  case Game::PRIME_1_GCN:
    run_mod_mp1_gc();
    break;
  case Game::PRIME_2:
    run_mod_mp2();
    break;
  case Game::PRIME_2_GCN:
    run_mod_mp2_gc();
    break;
  case Game::PRIME_3:
  case Game::PRIME_3_STANDALONE:
    run_mod_mp3();
    break;
  default:
    break;
  }
}

void ViewModifier::adjust_viewmodel(float fov, u32 arm_address, u32 znear_address, u32 znear_value) {
  //bool is_mp3_standalone_us = hack_mgr->get_active_game() == Game::PRIME_3_STANDALONE && hack_mgr->get_active_region() == Region::NTSC_U;
  float left = 0.25f;
  float forward = 0.30f;
  float up = -0.35f;
  bool apply_znear = false;

  if (GetToggleArmAdjust()) {
    if (GetAutoArmAdjust()) {
      if (fov > 125) {
        left = 0.22f;
        forward = -0.02f;
        apply_znear = true;
        //if (is_mp3_standalone_us) {
        //  forward = 0.05f;
        //  up = -0.36f;
        //}
      } else if (fov >= 75) {
        float factor = (fov - 75) / (125 - 75);
        left = Lerp(left, 0.22f, factor);
        forward = Lerp(forward, -0.02f, factor);
        //if (is_mp3_standalone_us) {
        //  up = Lerp(up, -0.48f, factor);
        //}
        apply_znear = true;
      }
    } else {
      std::tie<float, float, float>(left, forward, up) = GetArmXYZ();
      apply_znear = true;
    }
  }

  if (apply_znear) {
    write32(znear_value, znear_address);
  }

  writef32(left, arm_address);
  writef32(forward, arm_address + 0x4);
  writef32(up, arm_address + 0x8);
}

void ViewModifier::run_mod_mp1() {
  LOOKUP(static_fov_fp);
  LOOKUP(static_fov_tp);
  LOOKUP(gun_pos);

  LOOKUP_DYN(object_list);
  if (object_list == 0) {
    return;
  }

  LOOKUP_DYN(camera_manager);
  if (camera_manager == 0) {
    return;
  }

  u16 camera_id = read16(camera_manager);
  if (camera_id == 0xffff) {
    return;
  }

  const u32 camera = read32(object_list + ((camera_id & 0x3ff) << 3) + 4);
  if (!mem_check(camera)) {
    return;
  }

  const float fov = std::min(GetFov(), 170.f);
  writef32(fov, camera + 0x164);
  writef32(fov, static_fov_fp);
  writef32(fov, static_fov_tp);

  adjust_viewmodel(fov, gun_pos, camera + 0x168, 0x3d200000);
  set_code_group_state("culling", (GetCulling() || GetFov() > 101.f) ? ModState::ENABLED : ModState::DISABLED);

  DevInfo("camera", "%08X", camera);
}

void ViewModifier::run_mod_mp1_gc() {
  u8 version = read8(0x80000007);
  if (version != 0) {
    return;
  }

  LOOKUP(fov_fp_offset);
  LOOKUP(fov_tp_offset);
  LOOKUP(gun_pos);
  
  LOOKUP_DYN(object_list);
  if (object_list == 0) {
    return;
  }

  LOOKUP_DYN(camera_manager);
  if (camera_manager == 0) {
    return;
  }

  const u16 camera_uid = read16(camera_manager);
  if (camera_uid == 0xffff) {
    return;
  }

  const u32 camera = read32(object_list + ((camera_uid & 0x3ff) << 3) + 4);
  if (!mem_check(camera)) {
    return;
  }

  // PAL added some stuff related to SFX in CActor, affects all derived
  // since NTSC-J is based on PAL it also contains those
  const u32 pal_ntsc_j_offset = (hack_mgr->get_active_region() == Region::PAL || hack_mgr->get_active_region() == Region::NTSC_J) ? 0x10 : 0;

  const u32 r13 = GPR(13);
  const float fov = std::min(GetFov(), 170.f);
  writef32(fov, camera + 0x15c + pal_ntsc_j_offset);
  writef32(fov, r13 + fov_fp_offset);
  writef32(fov, r13 + fov_tp_offset);

  adjust_viewmodel(fov, gun_pos, camera + 0x160 + pal_ntsc_j_offset,
    0x3d200000);

  set_code_group_state("culling", (GetCulling() || GetFov() > 101.f) ? ModState::ENABLED : ModState::DISABLED);
  DevInfo("camera", "%08X", camera);
}

void ViewModifier::run_mod_mp2() {
  LOOKUP(state_manager);
  LOOKUP(tweakgun);

  LOOKUP_DYN(object_list);
  if (object_list == 0) {
    return;
  }

  LOOKUP_DYN(camera_manager);
  if (camera_manager == 0) {
    return;
  }

  if (read32(state_manager + 0x153c) != 1) {
    return;
  }

  u16 camera_id = read16(camera_manager);
  if (camera_id == 0xffff) {
    return;
  }
  const u32 camera = read32(object_list + ((camera_id & 0x3ff) << 3) + 4);
  
  const float fov = std::min(GetFov(), 170.f);
  writef32(fov, camera + 0x1e8);

  adjust_viewmodel(fov, read32(read32(tweakgun)) + 0x4c, camera + 0x1c4, 0x3d200000);

  set_code_group_state("culling", (GetCulling() || GetFov() > 101.f) ? ModState::ENABLED : ModState::DISABLED);
  DevInfo("camera", "%08X", camera);
}

void ViewModifier::run_mod_mp2_gc() {
  LOOKUP(state_manager);
  LOOKUP(tweakgun_offset);

  LOOKUP_DYN(world);
  if (world == 0) {
    return;
  }
  // World loading phase == 4 -> complete
  if (read32(world + 0x4) != 4) {
    return;
  }

  LOOKUP_DYN(camera_manager);
  if (camera_manager == 0) {
    return;
  }

  LOOKUP_DYN(object_list);
  if (object_list == 0) {
    return;
  }

  const u16 camera_id = read16(camera_manager);
  if (camera_id == 0xffff) {
    return;
  }

  const u32 camera = read32(object_list + ((camera_id & 0x3ff) << 3) + 4);
  const float fov = std::min(GetFov(), 170.f);
  writef32(fov, camera + 0x1f0);
  adjust_viewmodel(fov, read32(read32(GPR(13) + tweakgun_offset)) + 0x50, camera + 0x1cc, 0x3d200000);

  set_code_group_state("culling", (GetCulling() || GetFov() > 101.f) ? ModState::ENABLED : ModState::DISABLED);
  DevInfo("camera", "%08X", camera);
}

void ViewModifier::run_mod_mp3() {
  LOOKUP(state_manager);
  LOOKUP(tweakgun);

  LOOKUP_DYN(camera_manager);
  if (camera_manager == 0) {
    return;
  }

  LOOKUP_DYN(object_list);
  if (object_list == 0) {
    return;
  }

  // Until vmcalls are implemented, this code can't be executed early
  // enough not to cause a flicker as FOV gets updated on recognizing
  // a new camera
  const u16 camera_id = read16(camera_manager);
  if (camera_id == 0xffff) {
    return;
  }
  const float fov = std::min(GetFov(), 170.f);

  // This will be a temporary measure, these don't seem to change but
  // bugs could arise?
  const u16 known_camera_uids[] = {3, 4, 6};
  for (int i = 0; i < sizeof(known_camera_uids) / sizeof(u16); i++) {
    const u32 camera = read32(object_list + ((known_camera_uids[i] & 0x7ff) << 3) + 4);
    if (!mem_check(camera)) {
      return;
    }

    const u32 camera_params = read32(camera + 0x178);
    writef32(fov, camera_params + 0x1c);
    writef32(fov, camera_params + 0x18);
  }
  // best guess on the name here
  LOOKUP_DYN(perspective_info);
  adjust_viewmodel(fov, read32(read32(tweakgun)) + 0xe0, perspective_info + 0x8c, 0x3dcccccd);

  set_code_group_state("culling", (GetCulling() || GetFov() > 96.f) ? ModState::ENABLED : ModState::DISABLED);
  
  const u32 camera = read32(object_list + ((camera_id & 0x7ff) << 3) + 4);
  DevInfo("camera", "%08X", camera);
}

bool ViewModifier::init_mod(Game game, Region region) {
  switch (game) {
  case Game::PRIME_1:
    init_mod_mp1(region);
    break;
  case Game::PRIME_1_GCN:
    init_mod_mp1_gc(region);
    break;
  case Game::PRIME_2:
    init_mod_mp2(region);
    break;
  case Game::PRIME_2_GCN:
    init_mod_mp2_gc(region);
    break;
  case Game::PRIME_3:
    init_mod_mp3(region);
    break;
  case Game::PRIME_3_STANDALONE:
    init_mod_mp3_standalone(region);
    break;
  }
  return true;
}

void ViewModifier::init_mod_mp1(Region region) {
  if (region == Region::NTSC_U) {
    add_code_change(0x802c7dbc, 0x38600001, "culling");
    add_code_change(0x802c7dbc + 0x4, 0x4e800020, "culling");
  } else if (region == Region::PAL) {
    add_code_change(0x802c8024, 0x38600001, "culling");
    add_code_change(0x802c8024 + 0x4, 0x4e800020, "culling");
  } else if (region == Region::NTSC_J) {
    add_code_change(0x802c7a3c, 0x38600001, "culling");
    add_code_change(0x802c7a3c + 0x4, 0x4e800020, "culling");
  }
}

void ViewModifier::init_mod_mp1_gc(Region region) {
  u8 version = read8(0x80000007);

  if (region == Region::NTSC_U) {
    if (version == 0) {
      add_code_change(0x80337a24, 0x38600001, "culling");
      add_code_change(0x80337a24 + 0x4, 0x4e800020, "culling");
    }
  } else if (region == Region::NTSC_U) {
    add_code_change(0x803222f8, 0x38600001, "culling");
    add_code_change(0x803222f8 + 0x4, 0x4e800020, "culling");
  } else if (region == Region::PAL) {
    add_code_change(0x80320424, 0x38600001, "culling");
    add_code_change(0x80320424 + 0x4, 0x4e800020, "culling");
  }
}

void ViewModifier::init_mod_mp2(Region region) {
  if (region == Region::NTSC_U) {
    add_code_change(0x802c8114, 0x38600001, "culling");
    add_code_change(0x802c8114 + 0x4, 0x4e800020, "culling");
  } else if (region == Region::NTSC_J) {
    add_code_change(0x802c6a28, 0x38600001, "culling");
    add_code_change(0x802c6a28 + 0x4, 0x4e800020, "culling");
  } else if (region == Region::PAL) {
    add_code_change(0x802ca730, 0x38600001, "culling");
    add_code_change(0x802ca730 + 0x4, 0x4e800020, "culling");
  }
}

void ViewModifier::init_mod_mp2_gc(Region region) {
  if (region == Region::NTSC_U) {
    add_code_change(0x801b0b38, 0x60000000);
    add_code_change(0x802f84c0, 0x38600001, "culling");
    add_code_change(0x802f84c0 + 0x4, 0x4e800020, "culling");
  } else if (region == Region::NTSC_J) {
    add_code_change(0x801b28f0, 0x60000000);
    add_code_change(0x802faa28, 0x38600001, "culling");
    add_code_change(0x802faa28 + 0x4, 0x4e800020, "culling");
  } else if (region == Region::PAL) {
    add_code_change(0x801b0e44, 0x60000000);
    add_code_change(0x802f8818, 0x38600001, "culling");
    add_code_change(0x802f8818 + 0x4, 0x4e800020, "culling");
  }
}

void ViewModifier::init_mod_mp3(Region region) {
  if (region == Region::NTSC_U) {
    add_code_change(0x8007f3dc, 0x60000000);
    add_code_change(0x8009d058, 0x60000000);
    add_code_change(0x8031490c, 0x38600001, "culling");
    add_code_change(0x8031490c + 0x4, 0x4e800020, "culling");
  } else if (region == Region::PAL) {
    add_code_change(0x8007f3dc, 0x60000000);
    add_code_change(0x8009d058, 0x60000000);
    add_code_change(0x80314038, 0x38600001, "culling");
    add_code_change(0x80314038 + 0x4, 0x4e800020, "culling");
  }
}
  
void ViewModifier::init_mod_mp3_standalone(Region region) {
  if (region == Region::NTSC_U) {
    add_code_change(0x8007f504, 0x60000000);
    add_code_change(0x8009cfc4, 0x60000000);
    add_code_change(0x80316a1c, 0x38600001, "culling");
    add_code_change(0x80316a1c + 0x4, 0x4e800020, "culling");
  } else if (region == Region::PAL) {
    add_code_change(0x8007f7a0, 0x60000000);
    add_code_change(0x8009d41c, 0x60000000);
    add_code_change(0x80318170, 0x38600001, "culling");
    add_code_change(0x80318170 + 0x4, 0x4e800020, "culling");
  } else if (region == Region::NTSC_J) {
    add_code_change(0x8007f934, 0x60000000);
    add_code_change(0x8009d5b0, 0x60000000);
    add_code_change(0x8031a4b4, 0x38600001, "culling");
    add_code_change(0x8031a4b4 + 0x4, 0x4e800020, "culling");
  }
}
}  // namespace prime
