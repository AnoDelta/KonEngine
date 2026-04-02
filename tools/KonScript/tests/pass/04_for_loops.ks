// C-style for, for-in range (exclusive + inclusive), for-in array

func main() {
    let mut sum: I32 = 0;

    for i: I32 = 0; i < 10; i++ {
        sum += i;
    }

    for k: I32 in 0..5 {
        sum += k;
    }

    for k: I32 in 1..=5 {
        sum += k;
    }

    let nums: [I32] = [10, 20, 30, 40];
    for n: I32 in nums {
        sum += n;
    }
}
