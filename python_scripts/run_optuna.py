import math
import subprocess
import optuna

import logging
import sys
import tempfile
import pathlib
import os

import run_local

FILE_DIR = pathlib.Path(__file__).parent.absolute()
REPO_ROOT = FILE_DIR.parent.absolute()

# Add stream handler of stdout to show the messages
optuna.logging.get_logger("optuna").addHandler(logging.StreamHandler(sys.stdout))
study_name = sys.argv[1]  # Unique identifier of the study.
storage_name = "sqlite:///{}.db".format(study_name)

storage = optuna.storages.RDBStorage(
    url=storage_name, engine_kwargs={"connect_args": {"timeout": 100}}
)

study = optuna.create_study(
    study_name=study_name,
    storage=storage,
    load_if_exists=True,
    direction="minimize",
)


def gen_build_command(source_file: str, params):
    return f"g++ {str(source_file)} -std=c++20 -O3 -DLOCAL " + (
        " ".join([f"-DPARAM_{key}={val}" for key, val in params.items()])
    )

def run_remote(source_path: pathlib.Path, dir: pathlib.Path, build_cmd: str):
    subprocess.run(
        ["bash", str(FILE_DIR / "run_jobs.sh")],
        env={
            "LOCAL_SOURCE_FILE_PATH": str(source_path),
            "CONTAINER_SOURCE_FILE_PATH": "main.cpp",
            "GCS_INPUT_DIR_PATH": "gs://ahc019/in/500",
            "GCS_OUTPUT_DIR_PATH": "gs://ahc019/test/out",  # TODO: change path
            "BUILD_COMMAND": build_cmd,
            "LOCAL_OUTPUT_DIR_PATH": str(dir / "out"),
            "TASKS": "50",
        },
    ).check_returncode()


def objective(trial: optuna.Trial):
    seeds = [1000,
             trial.suggest_int("OP2_P", 0, 1000000),
             trial.suggest_int("OP3_P", 0, 1000000),
             trial.suggest_int("OP4_P", 0, 1000000),
             trial.suggest_int("OP5_P", 0, 1000000),
             ]
    total = sum(seeds)

    params = dict(
        OP1_P=1000 - sum([int(math.floor(1000 * seed / total)) for seed in seeds[1:]]),
        OP2_P=int(math.floor(1000 * seeds[1] / total)),
        OP3_P=int(math.floor(1000 * seeds[2] / total)),
        OP4_P=int(math.floor(1000 * seeds[3] / total)),
        OP5_P=int(math.floor(1000 * seeds[4] / total)),
    )



    dirname = tempfile.mkdtemp(suffix=study_name)

    dir = pathlib.Path(dirname)
    source_path = REPO_ROOT / "src" / "main.cpp"
    build_cmd = gen_build_command(source_path, params)

    (dir / "out").mkdir(exist_ok=True)

    if (os.environ.get("REMOTE")):
        run_remote(source_path, dir, build_cmd)
    else:
        with tempfile.TemporaryDirectory(prefix=f"trial{trial.number}") as tmpdirname:
            bin = tmpdirname + "/a.out"
            # build
            subprocess.run(
                build_cmd + f" -o {tmpdirname}/a.out", shell=True
            ).check_returncode()

            # prepare dir
            (dir / "out").mkdir(exist_ok=True)
            (dir / "log").mkdir(exist_ok=True)

            # run
            results = run_local.execute_all(
                bin,
                "in/0-299",
                dir / "out",
                dir / "log",
                timeout=15,
                parallelism=10,
            )

            scores = run_local.parse_scores(results)
            average = sum(scores) / len(scores)


    return average;



study.enqueue_trial(
    {
        "OP2_P": 166,
        "OP3_P": 166,
        "OP4_P": 166,
        "OP5_P": 166,
    }
)

study.optimize(
    objective,
    n_trials=100,
)
