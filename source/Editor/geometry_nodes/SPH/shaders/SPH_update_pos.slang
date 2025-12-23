#include "utils.slangh"

ConstantBuffer<SPHConstants> sph_constants : register(b0);

RWByteAddressBuffer positions : register(u0);
RWByteAddressBuffer velocities : register(u1);

ByteAddressBuffer pressure : register(t0);
ByteAddressBuffer viscosity : register(t1);

// particle_count
[numthreads(32, 1, 1)]
void main(int2 i_globalIdx: SV_DispatchThreadID)
{
    if (i_globalIdx.x >= sph_constants.numParticles)
        return;

    float3 gravity = float3(0, sph_constants.gravity, 0);

    float dt = sph_constants.dt;
    int i = i_globalIdx.x;
    int3 iF_pres = int3(
        pressure.Load(i * 12),
        pressure.Load(i * 12 + 4),
        pressure.Load(i * 12 + 8));
    int3 iF_visc = int3(
        viscosity.Load(i * 12),
        viscosity.Load(i * 12 + 4),
        viscosity.Load(i * 12 + 8));
    float3 F_pres = float3(iF_pres) / INT_SCALE;
    float3 F_visc = float3(iF_visc) / INT_SCALE;
    float3 pos = asfloat(positions.Load3(i * 12));
    float3 vel = asfloat(velocities.Load3(i * 12));
    float3 pos_o = pos;

    // Update velocity and position
    vel += dt * (F_pres + F_visc + gravity);
    pos += dt * vel;

    // Simple boundary collision with axis-aligned box [0,1]^3
    float3 boundary = float3(1, 1, 1);

    // Check collision with min boundary (0,0,0)
    int3 minflag = int3(pos < float3(-1, -1, -1));
    // Check collision with max boundary
    int3 maxflag = int3(pos > boundary);

    // Reflect position if out of bounds
    pos = -2 * pos * minflag + pos;
    pos = -2 * (pos - boundary) * maxflag + pos;

    // Apply damping to velocity on collision (coefficient 0.7)
    vel = (1 - 2 * (minflag + maxflag) * sqrt(0.7)) * vel;

    // Recalculate velocity from actual position change
    vel = (pos - pos_o) / dt;

    // Store updated values
    positions.Store3(i * 12, asuint(pos));
    velocities.Store3(i * 12, asuint(vel));
}
