#!/usr/bin/env python3
import logging
import sys
from argparse import ArgumentParser, ArgumentDefaultsHelpFormatter
from datetime import datetime
import matplotlib
import matplotlib.pyplot as plt
from matplotlib.dates import DateFormatter
matplotlib.use('agg')
plt.style.use('seaborn')


def _read_log_file(filename, from_date, to_date):
    with open(filename) as fd:
        logs = fd.read()

    data = []
    for line in logs.splitlines():
        if ("CNR" not in line):
            continue

        date = datetime.strptime(line[:19], "%Y-%m-%d %H:%M:%S")

        if (from_date is not None and to_date is not None and
                (date < from_date or date > to_date)):
            continue

        data.append({
            'date': date,
            'cnr': float(line.split("\t")[-1].replace("CNR: ", ""))
        })

    return data


def _plot(data):
    cnr = [x['cnr'] for x in data]
    date = [x['date'] for x in data]

    formatter = DateFormatter('%m/%d %H:%M')
    fig, ax = plt.subplots()
    plt.plot_date(date, cnr, ms=1)
    plt.ylabel("CNR (dB)")
    plt.xlabel("Time (h)")
    ax.xaxis.set_major_formatter(formatter)
    ax.xaxis.set_tick_params(rotation=30, labelsize=10)
    plt.tight_layout()
    plt.savefig("beacon_cnr.png", dpi=300)
    plt.close()


def parser():
    parser = ArgumentParser(
        prog="plot",
        description="Plot beacon CNR from log file",
        formatter_class=ArgumentDefaultsHelpFormatter
    )
    parser.add_argument(
        'file',
        help="Log file"
    )
    parser.add_argument(
        '--from-date',
        default=None,
        type=lambda s: datetime.strptime(s, '%Y-%m-%d %H:%M:%S'),
        help="Starting timestamp to plot"
    )
    parser.add_argument(
        '--to-date',
        default=None,
        type=lambda s: datetime.strptime(s, '%Y-%m-%d %H:%M:%S'),
        help="Ending timestamp to plot"
    )
    parser.add_argument(
        '--verbose', '-v',
        action='count',
        default=1,
        help="Verbosity (logging) level"
    )
    return parser.parse_args()


def main():
    args = parser()

    logging_level = 70 - (10 * args.verbose) if args.verbose > 0 else 0
    logging.basicConfig(stream=sys.stderr, level=logging_level)

    data = _read_log_file(args.file, args.from_date, args.to_date)
    _plot(data)


if __name__ == '__main__':
    main()
