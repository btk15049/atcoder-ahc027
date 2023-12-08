import os
import subprocess
from dataclasses import dataclass
import sys
from time import time
from pathlib import Path
from typing import List, Optional
from joblib import Parallel, delayed
import math
import last_score


@dataclass
class ExecuteResult:
    input_file: Path
    output_file: Path
    log_file: Path
    elapsed: float
    message: str

    def is_succeeded(self) -> bool:
        return not bool(self.message)


def execute_command(
    command, input_file, output_file, log_file, timeout=None
) -> ExecuteResult:
    start_time = time()
    message = ""
    try:
        result = subprocess.run(
            command,
            stdin=open(input_file, "r"),
            stdout=open(output_file, "w"),
            stderr=open(log_file, "w"),
            timeout=timeout,
        )
        result.check_returncode()
    except subprocess.TimeoutExpired as e:
        message = f"実行中にタイムアウトが発生しました: {timeout} 秒を超えました。"
    except subprocess.CalledProcessError as e:
        message = f"実行中にエラーが発生しました: コマンドが非ゼロのステータスで終了しました。終了コード: {e.returncode}"
    except Exception as e:
        message = f"実行中にエラーが発生しました: {e}"
    finally:
        end_time = time()
        elapsed = end_time - start_time

    return ExecuteResult(input_file, output_file, log_file, elapsed, message)


def execute_all(
    command, input_dir, output_dir, log_dir, timeout=None, parallelism=1
) -> List[ExecuteResult]:
    input_dir = Path(input_dir)
    output_dir = Path(output_dir)
    log_dir = Path(log_dir)

    output_dir.mkdir(parents=True, exist_ok=True)
    log_dir.mkdir(parents=True, exist_ok=True)

    input_files = [input_dir / file_name for file_name in os.listdir(input_dir)]

    results = Parallel(n_jobs=parallelism, verbose=10)(
        delayed(execute_command)(
            command,
            input_file,
            output_dir / input_file.name,
            log_dir / input_file.name,
            timeout,
        )
        for input_file in input_files
    )
    results = sorted(results, key=lambda r: r.input_file.name)

    failed_cases = [
        result.input_file.resolve() for result in results if not result.is_succeeded()
    ]
    elapsed_sorted: list[ExecuteResult] = sorted(results, key=lambda x: x.elapsed)
    n = len(elapsed_sorted)

    print(f"実行に失敗したテストケース: {len(failed_cases)}/{len(results)}")
    for failed_case in failed_cases:
        print(f"  {failed_case}")

    print(
        f"実行時間の最大: {elapsed_sorted[-1].elapsed:.2f} 秒, {elapsed_sorted[-1].input_file}"
    )
    print(f"実行時間の上位 5%: {elapsed_sorted[int(n * 0.95)].elapsed:.2f} 秒")
    print(f"実行時間の上位 50%: {elapsed_sorted[int(n * 0.50)].elapsed:.2f} 秒")
    print(f"実行時間の上位 75%: {elapsed_sorted[int(n * 0.25)].elapsed:.2f} 秒")

    return results


def parse_value(result: ExecuteResult, key: str) -> Optional[str]:
    val: Optional[str] = None
    prefix = f"[{key}]"
    with open(result.log_file, "r") as f:
        lines = f.readlines()
        for line in lines:
            if line.startswith(prefix):
                val = line[len(prefix) :].strip()
    return val

def parse_score(result: ExecuteResult) -> Optional[float]:
    # to float
    return float(parse_value(result, "score"))

def parse_scores(results: List[ExecuteResult]) -> List[float]:
    scores = []
    for result in results:
        score = parse_score(result)
        if score is None:
            print(f"{result.log_file} のパースに失敗しました。")
            scores.append(100)
        else:
            scores.append(math.log2(score))
    return scores

if __name__ == "__main__":
    repo_root_path = Path(__file__).resolve().parent.parent
    results = execute_all(
        ["./build/bin/a.out"],
        repo_root_path / "in" / sys.argv[1],
        repo_root_path / "out" / sys.argv[1],
        repo_root_path / "log" / sys.argv[1],
        timeout=15,
        parallelism=10,
    )
    scores = parse_scores(results)

    if len(scores) != 0:
        average = sum(scores) / len(scores)
        print(f"Average: {last_score.LAST_LOG_SCORE} -> {average}")
        diff = last_score.LAST_LOG_SCORE - average
        print(f"Estimated Score: {last_score.LAST_REL_SCORE} -> {last_score.LAST_REL_SCORE * (2 ** diff)}")

    print(f"Valid Cases: {len(scores)}")
