import sys
import os
sys.path.append(os.path.abspath("."))
import pyrd53.pybindings
import psutil

enc = pyrd53.pybindings.EventEncoder("example/itdata.root")
counter = 0
for i in range(1):
    print("Event # ", i)
    ev = 0
    print("\tRAM used memory - ", psutil.virtual_memory().percent, "%", ", Swap used memory - ", psutil.swap_memory().percent, "%")
    ev = enc.get_next_event()
    if(not ev.is_empty()):
        ch = ev.get_next_chip()
        while (len(ch[1]) > 0):
            # check mistamtch
            raw_hits_with_adc = ev.get_chip_hits(ch[0])
            raw_hits = []
            for raw_hit_with_adc in raw_hits_with_adc:
                raw_hits.append(raw_hit_with_adc[0])
            cluster_hits = []
            for cluster in ev.get_chip_clusters(ch[0]):
                cluster_hits += cluster.get_hits()
            match = (len(raw_hits) == len(cluster_hits))
            if (not match):
                error_string = "Explanation:\n"
                print("\t\t\tNumber of RAW and Cluster hits does not match, skipping this chip")
                if len(cluster_hits) > len(raw_hits):
                    for cluster_hit in cluster_hits:
                        if not (cluster_hit in raw_hits):
                            error_string += ("\t\tCluster: (col " + str((cluster_hit >> 0) & 0xffff) + ", row " + str((cluster_hit >> 16) & 0xffff) + ") not in raw hits\n")
                else:
                    for raw_hit in raw_hits:
                        if not (raw_hit in cluster_hits):
                            error_string += ("\t\tRaw hit: (col " + str((raw_hit >> 0) & 0xffff) + ", row " +
                                  str((raw_hit >> 16) & 0xffff) + ") not in cluster hits\n")
                error_string += ("Event id raw: " + str(ev.get_event_id_raw()) + "\n")
                error_string += ev.chip_str(ch[0])
                # write
                f = open("./failed/event_" + str(counter) + ".txt", "w")
                f.write(error_string)
                f.close()

            # increment count
            counter += 1
            # get next
            ch = ev.get_next_chip()
    else:
        break
print("Total: ", counter)

