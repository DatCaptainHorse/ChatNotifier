// Interval with limited number of calls
export function intervalLimited(callback: () => void, interval: number, limit: number) {
  let count = 0;
  const intervalId = setInterval(() => {
    if (count >= limit) {
      clearInterval(intervalId);
    } else {
      count++;
      callback();
    }
  }, interval);
}
