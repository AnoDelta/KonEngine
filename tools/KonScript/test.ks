#include <engine>

node Player : Node2D {
    let speed: F64 = 200.0;
    let mut health: I32 = 100;

    func Update(dt: F64) {
        if KeyDown(32) { x += speed * dt; }
        if health <= 0 { Emit("player_dead"); }
    }

    func TakeDamage(amount: I32) {
        health -= amount;
    }
}
