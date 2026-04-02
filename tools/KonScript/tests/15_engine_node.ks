// 15_engine_node.ks — engine target: node declarations, lifecycle hooks
// Run with: konscript --check 15_engine_node.ks
#include <engine>

const SPEED:    F64 = 250.0;
const MAX_HP:   I32 = 100;
const JUMP_VEL: F64 = -400.0;

enum PlayerState {
    Idle,
    Running,
    Jumping,
    Dead,
}

node Player : Node2D {
    let mut hp:     I32 = MAX_HP;
    let mut vel_y:  F64 = 0.0;
    let mut state:  PlayerState = Idle;
    let mut grounded: Bool = true;
    let score: I32 = 0;

    func Ready() {
        x = 100.0;
        y = 300.0;
    }

    func Update(dt: F64) {
        // Horizontal movement
        if KeyDown(Key.D) { x += SPEED * dt; }
        if KeyDown(Key.A) { x -= SPEED * dt; }

        // Jump
        if KeyPressed(Key.Space) && grounded {
            vel_y = JUMP_VEL;
            grounded = false;
        }

        // Gravity
        vel_y += 980.0 * dt;
        y += vel_y * dt;

        // Simple floor
        if y >= 400.0 {
            y = 400.0;
            vel_y = 0.0;
            grounded = true;
        }

        // State machine
        switch state {
            case Idle:
                if KeyDown(Key.D) || KeyDown(Key.A) { state = Running; }
            case Running:
                if !KeyDown(Key.D) && !KeyDown(Key.A) { state = Idle; }
            case Jumping:
                if grounded { state = Idle; }
            case Dead:
                return;
            default:
                state = Idle;
        }

        // Out of bounds
        if hp <= 0 {
            state = Dead;
            Emit("player_dead");
        }
    }

    func TakeDamage(amount: I32) {
        hp -= amount;
        if hp < 0 { hp = 0; }
    }

    func Heal(amount: I32) {
        hp += amount;
        if hp > MAX_HP { hp = MAX_HP; }
    }

    func IsAlive() -> Bool {
        return hp > 0;
    }
}

node Enemy : Node2D {
    let mut speed: F64 = 80.0;
    let mut hp:    I32 = 30;

    func Update(dt: F64) {
        x -= speed * dt;
        if x < -50.0 { x = 900.0; }
    }

    func OnCollisionEnter(other: Collider2D) {
        hp -= 10;
        if hp <= 0 {
            Emit("enemy_dead");
        }
    }
}

func main() {
    InitWindow(800, 600, "KonEngine Test");
    SetTargetFPS(60);

    let mut scene: Scene = Scene();
    let player: Player = scene.add(Player, "player");
    let enemy:  Enemy  = scene.add(Enemy,  "enemy");

    while !WindowShouldClose() {
        let dt: F64 = GetDeltaTime();
        ClearBackground(0.1, 0.1, 0.15);
        scene.update(dt);
        scene.draw();
        Present();
        PollEvents();
    }
}
