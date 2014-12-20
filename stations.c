#include <stdio.h>
#include <string.h>

const char *credits =
	"***                                                                     ***\n"
	"*** Data provided by NJ TRANSIT, which is the sole owner of the Data.   ***\n"
	"***                                                                     ***\n";

const char *stations[] = {
"Anderson Street","Annandale","Asbury Park","Atco","Atlantic City","Avenel",
"Basking Ridge","Bay Head","Bay Street","Belmar","Berkeley Heights",
"Bernardsville","Bloomfield","Boonton","Bound Brook","Bradley Beach",
"Brick Church","Bridgewater","Broadway","Campbell Hall","Chatham",
"Cherry Hill","Clifton","Convent","Cranford","Delawanna","Denville",
"Dover","Dunellen","East Orange","Edison","Egg Harbor City","Elberon",
"Elizabeth","Emerson","Essex Street","Fanwood","Far Hills","Garfield",
"Garwood","Gillette","Gladstone","Glen Ridge","Glen Rock Boro Hall",
"Glen Rock Main Line","Hackettstown","Hamilton","Hammonton","Harriman",
"Hawthorne","Hazlet","High Bridge","Highland Avenue","Hillsdale","Ho-Ho-Kus",
"Hoboken","Jersey Avenue","Kingsland","Lake Hopatcong","Lebanon","Lincoln Park",
"Linden","Lindenwold","Little Falls","Little Silver","Long Branch","Lyndhurst",
"Lyons","Madison","Mahwah","Manasquan","Maplewood","Metropark","Metuchen",
"Middletown New Jersey","Middletown New York","Millburn","Millington",
"Monmouth Park","Montclair Heights","Montclair State University","Montvale",
"Morris Plains","Morristown","Mount Arlington","Mount Olive","Mount Tabor",
"Mountain Avenue","Mountain Lakes","Mountain","Mountain View","Murray Hill",
"Nanuet","Netcong","Netherwood","New Bridge Landing","New Brunswick",
"New Providence","New York Penn","Newark Airport","Newark Broad Street",
"Newark Penn","North Branch","North Elizabeth","Oradell","Orange","Otisville",
"Park Ridge","Passaic","Paterson","Peapack","Pearl River",
"Pennsauken Transit Center","Perth Amboy","Philadelphia 30th Street",
"Plainfield","Plauderville","Point Pleasant Beach","Port Jervis","Princeton",
"Princeton Junction","Radburn","Rahway","Ramsey","Ramsey Route 17","Raritan",
"Red Bank","Ridgewood","River Edge","Roselle Park","Rutherford",
"Salisbury Mills Cornwall","Secaucus Upper","Short Hills","Sloatsburg",
"Somerville","South Amboy","South Orange","Spring Lake","Spring Valley",
"Stirling","Suffern","Summit","Teterboro","Towaco","Trenton Transit Center",
"Tuxedo","Union","Upper Montclair","Waldwick","Walnut Street","Watchung Avenue",
"Watsessing Avenue","Wayne/Route 23 Transit Center","Westfield","Westwood",
"White House","Wood Ridge","Woodbridge","Woodcliff Lake","Secaucus Lower"
};

const char *codes[] = {
"AM","AB","AZ","AH","AS","AN","AP","AO","AC","AV","BI","BH","MC","BS","BY",
"BV","BM","BN","BK","BB","BU","BW","BF","CB","CM","CY","IF","CN","XC","DL",
"DV","DO","DN","EO","ED","EH","EL","EZ","EN","EX","FW","FH","GD","GW","GI",
"GL","GG","GK","RS","HQ","HL","HN","RM","HW","HZ","HG","HI","HD","UF","HB",
"JA","KG","HP","ON","LP","LI","LW","FA","LS","LB","LN","LY","MA","MZ","SQ",
"MW","MP","MU","MI","MD","MB","GO","MK","HS","UV","ZM","MX","MR","HV","OL",
"TB","MS","ML","MT","MV","MH","NN","NT","NE","NH","NB","NV","NY","NA","ND",
"NP","OR","NZ","OD","OG","OS","PV","PS","RN","PC","PQ","PN","PE","PH","PF",
"PL","PP","PO","PR","PJ","FZ","RH","RY","17","RA","RB","RW","RG","RL","RF",
"CW","SE","RT","XG","SM","CH","SO","LA","SV","SG","SF","ST","TE","TO","TR",
"TC","US","UM","WK","WA","WG","WT","23","WF","WW","WH","WR","WB","WL","TS"
};

const size_t n_stations = sizeof(stations) / sizeof(stations[0]);

void
stations_list()
{
	int i;
	for (i = 0; i < n_stations; i++) {
		printf("%-40s    %2s\n", stations[i], codes[i]);
	}

}

size_t
station_index(const char *code)
{
	int i;
	for (i = 0; i < n_stations; i++) {
		if (strcmp(code, codes[i]) == 0)
			return i;
	}
	return -1;
}

const char *
station_name(const char *code)
{
	int i;
	for (i = 0; i < n_stations; i++) {
		if (strcmp(code, codes[i]) == 0)
			return stations[i];
	}
	return NULL;
}
