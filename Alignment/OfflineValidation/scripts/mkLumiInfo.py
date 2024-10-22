#!/usr/bin/env python3

from argparse import ArgumentParser
import logging
import os
import csv
import subprocess

NORMTAG_FILES = {
    'delivered': {
        'default': '/cvmfs/cms-bril.cern.ch/cms-lumi-pog/Normtags/normtag_BRIL.json'
    },
    'recorded': {
        'default': '/cvmfs/cms-bril.cern.ch/cms-lumi-pog/Normtags/normtag_PHYSICS.json'
    }
}


def main(args, unknown):
    logging.info('args = %s', args)
    logging.info('unkn = %s', unknown)

    for year in args.years:
        fname_csv = 'lumiperrun%d.csv' %(year%100)  # os.path.join(os.environ['CMSSW_BASE'], 'src', 'Alignment', 'OfflineValidation', 'data', )
        run_bril(year, fname_csv)

    return 0


def run_bril(year, out_csv):
    # Select normtag
    normtag_files = NORMTAG_FILES['delivered']
    normtag = normtag_files.get(year, normtag_files['default'])

    # Call brilcalc
    brilcalc_env = '. /cvmfs/cms-bril.cern.ch/cms-lumi-pog/brilws-docker/brilws-env'
    brilcalc_cmd = [
        'brilcalc',
        'lumi',
        '-u', '/pb',
        '-b', 'STABLE BEAMS',
        '-o', out_csv,
        # '-c', 'web',
        '--amodetag', 'PROTPHYS',
        '--begin', '01/01/%d 00:00:00' %(year%100),
        '--end'  , '12/31/%d 23:59:59' %(year%100),
        '--normtag', normtag
    ]
    brilcalc_cmd.extend(unknown)
    # logging.debug('brilcalc_cmd: %s', ' '.join(brilcalc_cmd))

    # Cannot use shell=False and have both the environment setup and the actual brilcalc command executed in the same shell
    brilcalc_cmd_joined = brilcalc_env+'\n'
    for word in brilcalc_cmd:
        # Very rough space escaping, except if there are already quotes
        if(' ' in word and not ('"' in word or "'" in word)):
            word = '"%s"' %(word)
        brilcalc_cmd_joined += word + ' '
    logging.debug('brilcalc_cmd_joined: %s', brilcalc_cmd_joined)
    subprocess.check_call(brilcalc_cmd_joined, shell=True)
    logging.info('wrote %s', out_csv)


def parse_args():
    parser = ArgumentParser(
        description='Retrieve information on runs and relative integrated luminosities using brilcalc.',
        epilog='Unrecognized arguments are passed verbatim to brilcalc.'
    )
    parser.add_argument('-y', '--years', nargs='+', type=int, default=[2022, 2023, 2024], help='Default: %(default)s.')
    parser.add_argument(      '--lumi-type', choices=['delivered', 'recorded'], default='delivered', help='Default: %(default)s')
    parser.add_argument('--log', dest='loglevel', metavar='LEVEL', default='WARNING', help='Level for the python logging module. Can be either a mnemonic string like DEBUG, INFO or WARNING or an integer (lower means more verbose).')

    return parser.parse_known_args()


if __name__ == '__main__':
    args, unknown = parse_args()
    loglevel = args.loglevel.upper() if not args.loglevel.isdigit() else int(args.loglevel)
    logging.basicConfig(format='%(levelname)s:%(module)s:%(funcName)s: %(message)s', level=loglevel)

    exit(main(args, unknown))
