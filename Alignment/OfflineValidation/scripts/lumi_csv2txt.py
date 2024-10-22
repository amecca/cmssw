#!/usr/bin/env python3

from argparse import ArgumentParser
import logging
import csv

def main(args):
    logging.debug('args = %s', args)

    outfmode = 'w' if args.force else 'x' # eXclusive creation
    for infname in args.filenames:
        outfname = infname.removesuffix('.csv')+'.txt'
        with open(infname, 'r') as fin, open(outfname, outfmode) as fout:
            convert(fin, fout, lumi_type=args.lumi_type)
        logging.info('%s -> %s', infname, outfname)

    return 0


def convert(fin, fout, lumi_type='delivered'):
    # Read and parse manually the first line
    logging.debug('1st line: %s', fin.readline().rstrip('\n'))
    header = fin.readline().rstrip('\n').lstrip('# ').split(',')
    logging.debug('header: %s', header)

    # Find the indices of columns of interest
    delivered_idx = None
    recorded_idx  = None
    for i,h in enumerate(header):
        if 'delivered' in h:
            delivered_idx = i
        if 'recorded' in h:
            recorded_idx  = i

    logging.debug('delivered_idx: %d', delivered_idx)
    logging.debug('recorded_idx : %d', recorded_idx)
    if  (lumi_type == 'delivered'):
        lumi_idx = delivered_idx
    elif(lumi_type == 'recorded' ):
        lumi_idx = recorded_idx
    logging.info('Using "%s" luminosity, column %d)', header[lumi_idx], lumi_idx)

    # Parse the rest of the csv with csv.reader
    reader = csv.reader(fin) #csv.DictReader(fin, fieldnames=header)
    for row in reader:
        if(row[0].startswith('#')):
            logging.debug(','.join(row))
            continue

        run  = int(row[0].split(':')[0])
        lumi = row[lumi_idx]
        fout.write('%d %s\n' %(run, lumi))


def parse_args():
    parser = ArgumentParser(
        description='Convert integrated luminosities from brilcalc (CSV) to the plain txt format (run luminosity) used by plotting scripts.'
    )
    parser.add_argument('filenames', nargs='+', metavar='FILE')
    parser.add_argument('-f', '--force', action='store_true', help='Overwrite existing files (default: %(default)s).')
    parser.add_argument('-l', '--lumi', dest='lumi_type', choices=['delivered', 'recorded'], default='delivered', help='Default: %(default)s')
    parser.add_argument('--log', dest='loglevel', metavar='LEVEL', default='WARNING', help='Level for the python logging module. Can be either a mnemonic string like DEBUG, INFO or WARNING or an integer (lower means more verbose).')

    return parser.parse_args()


if __name__ == '__main__':
    args = parse_args()
    loglevel = args.loglevel.upper() if not args.loglevel.isdigit() else int(args.loglevel)
    logging.basicConfig(format='%(levelname)s:%(module)s:%(funcName)s: %(message)s', level=loglevel)

    exit(main(args))
