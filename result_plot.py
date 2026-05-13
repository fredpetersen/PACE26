import matplotlib as mpl
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import sys
import os

run_id = sys.argv[1] if len(sys.argv) > 1 else "latest"
TIMEOUT_S = 30  # treated as ceiling for plotting

# summary.json is not valid json, but it is a sequence of json objects, one per line, so we can read it with pandas by treating each line as a separate json object.
path_name = f"stride-logs/{run_id}/summary.json"
df = pd.read_json(path_name, lines=True)

# Optional: optimal k for instances we couldn't solve.
# CSV format expected: id,optimal_k  (id = integer matching exact{NN}.nw).
# We map id -> idigest by parsing the `#s idigest "..."` header line of each
# pace26_exact_pub/exact{NN}.nw file.
optil_path = "optil_results.csv"
exact_dir = "pace26_exact_pub"
if os.path.exists(optil_path) and os.path.isdir(exact_dir):
    import re
    id_to_digest = {}
    digest_re = re.compile(r'idigest\s+"([0-9a-f]+)"')
    for fn in os.listdir(exact_dir):
        m = re.match(r"exact(\d+)\.nw$", fn)
        if not m:
            continue
        nid = int(m.group(1))
        with open(os.path.join(exact_dir, fn), "r") as fh:
            # idigest is on line 2 in current files, but scan first ~5 lines
            # to be robust to header tweaks.
            for _ in range(5):
                line = fh.readline()
                if not line:
                    break
                dm = digest_re.search(line)
                if dm:
                    id_to_digest[nid] = dm.group(1)
                    break
    optil = pd.read_csv(optil_path)
    optil.columns = [c.lower() for c in optil.columns]
    optil["idigest"] = optil["id"].map(id_to_digest)
    missing = optil["idigest"].isna().sum()
    if missing:
        print(f"warn: {missing} optil rows had no matching exact{{NN}}.nw")
    df = df.merge(optil[["idigest", "optimal_k"]],
                  left_on="s_idigest", right_on="idigest", how="left")
else:
    df["optimal_k"] = np.nan

df["solved"] = df["s_result"] == "Valid"
# best_k = our score if solved, else the known optimal (if available)
df["best_k"] = np.where(df["solved"], df["s_score"], df["optimal_k"])
df["wtime_capped"] = np.where(df["solved"], df["s_wtime"], TIMEOUT_S)

# Example line from summary.json:
# {"s_solver_path":"/Users/fp/school/PACE26/PACE26/pace26.out","s_key":"21c6a32c36b3f900fdc101cb0ca43b6b","s_path":"/Users/fp/school/PACE26/PACE26/stride-downloads/21/c6/a32c36b3f900fdc101cb0ca43b6b","s_idigest":"21c6a32c36b3f900fdc101cb0ca43b6b","s_num_trees":6,"s_num_leaves":28,"s_prev_best":20,"s_heuristic_score":1.0,"s_result":"Valid","s_score":20,"s_wtime":3.533878542,"s_utime":3.311558,"s_stime":0.016108,"s_maxrss":18399232,"s_minflt":1197,"s_majflt":1,"s_nvcsw":1,"s_nivcsw":3733}

# Failed example line from summary.json:
# {"s_solver_path":"/Users/fp/school/PACE26/PACE26/pace26.out","s_key":"13ff771b3a2ed03bb76a14507f1b9252","s_path":"/Users/fp/school/PACE26/PACE26/stride-downloads/13/ff/771b3a2ed03bb76a14507f1b9252","s_idigest":"13ff771b3a2ed03bb76a14507f1b9252","s_num_trees":3,"s_num_leaves":109,"s_result":"Timeout"}


# ---------------------------------------------------------------------------
# Figure with 3 panels:
#   (1) Instance landscape: trees x leaves, colored by solved/timeout, sized by k
#   (2) Solvability vs k: % solved within timeout, bucketed by k
#   (3) Wall time vs problem size, with k as color
# ---------------------------------------------------------------------------
fig, axes = plt.subplots(1, 3, figsize=(20, 6))

# --- (1) Instance landscape -------------------------------------------------
ax = axes[0]
solved = df[df["solved"]]
timeout = df[~df["solved"]]
# size proportional to known k (use best_k so timeouts with optil are sized too)
size_solved = 20 + 4 * solved["best_k"].fillna(0)
size_timeout = 20 + 4 * timeout["best_k"].fillna(timeout["s_num_leaves"] / 2)
ax.scatter(solved["s_num_leaves"], solved["s_num_trees"],
           s=size_solved, c="green", alpha=0.6, label="Solved",
           edgecolors="black", linewidths=0.3)
ax.scatter(timeout["s_num_leaves"], timeout["s_num_trees"],
           s=size_timeout, c="red", alpha=0.6, label="Timeout",
           marker="x", linewidths=1.2)
ax.set_xscale("log")
ax.set_xlabel("# leaves (log)")
ax.set_ylabel("# trees")
ax.set_title("Instance landscape (dot size ∝ k)")
ax.legend(loc="upper left")
ax.grid(True, alpha=0.3)

# --- (2) Solvability vs k ---------------------------------------------------
ax = axes[1]
# Use best_k where available; fall back to s_num_leaves only if absolutely needed
plotdf = df.dropna(subset=["best_k"]).copy()
plotdf["best_k"] = plotdf["best_k"].astype(int)
# bucket k into a handful of bins so each bar has a meaningful sample size
max_k = int(plotdf["best_k"].max())
bins = [0, 5, 10, 20, 25, 30, max_k + 1]
bins = sorted(set(b for b in bins if b <= max_k + 1))
labels = [f"{bins[i] + 1}-{bins[i+1]}" for i in range(len(bins) - 1)]
plotdf["k_bin"] = pd.cut(plotdf["best_k"], bins=bins, labels=labels, include_lowest=False)
agg = plotdf.groupby("k_bin", observed=True).agg(
    total=("solved", "size"), solved=("solved", "sum")
).reset_index()
agg["pct"] = 100 * agg["solved"] / agg["total"]
bars = ax.bar(agg["k_bin"].astype(str), agg["pct"], color="steelblue",
              edgecolor="black")
for bar, total, solved_ct in zip(bars, agg["total"], agg["solved"]):
    ax.text(bar.get_x() + bar.get_width() / 2,
            bar.get_height() + 1,
            f"{int(solved_ct)}/{int(total)}",
            ha="center", va="bottom", fontsize=9)
ax.set_xlabel("optimal k bucket")
ax.set_ylabel("% solved within timeout")
ax.set_ylim(0, 110)
ax.set_title("Solvability vs k")
ax.grid(True, axis="y", alpha=0.3)

# --- (3) Wall time vs leaves, colored by k ---------------------------------
ax = axes[2]
finite_k = df.dropna(subset=["best_k"])
finite_k = finite_k[finite_k["solved"]]
sc = ax.scatter(finite_k["s_num_leaves"], finite_k["wtime_capped"],
                c=finite_k["best_k"], cmap="viridis",
                s=20 + 3 * finite_k["s_num_trees"],
                alpha=0.75, edgecolors="black", linewidths=0.3)
ax.axhline(TIMEOUT_S, color="red", linestyle="--", alpha=0.4, label=f"{TIMEOUT_S}s timeout")
ax.set_xscale("log")
ax.set_yscale("log")
ax.set_xlabel("# leaves (log)")
ax.set_ylabel("wall time, s (log)")
ax.set_title("Time vs size (color = k, dot size ∝ #trees)")
plt.colorbar(sc, ax=ax, label="optimal k")
ax.legend(loc="lower right")
ax.grid(True, which="both", alpha=0.3)

plt.suptitle(f"Solver performance — run {run_id}  "
             f"(solved {int(df['solved'].sum())}/{len(df)})",
             fontsize=13)
plt.tight_layout()
# plt.show()
plt.savefig(f"result_plot_{run_id}.png", dpi=300)
