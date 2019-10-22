import os
import csv
import time
import shutil
from PyQt5.QtCore import QDate, QTime, QDateTime, Qt
 
 
CLEAR_DIR = True
 
 
class csvSystem:
 
    def __init__(self, Tracks, Drivers, Fields, BAUD_C, BAUD_X, BAUD_S):
        super().__init__()
 
        self.date = time.strftime('%y_%m_%d')
        self.time = time.strftime('%H:%M:%S')
        self.Fields = Fields
        self.Tracks = Tracks
        self.Track = None
        self.Drivers = Drivers
        self.Driver = None
        self.test_number = 0
        self.BAUD_C = BAUD_C
        self.BAUD_X = BAUD_X
        self.BAUD_S = BAUD_S
        self.build_dir()
        # self.writer , self.dictWriter = self.csv_creator()
 
 
    def build_dir(self):
 
        if CLEAR_DIR:
            try:
                shutil.rmtree('data/')
            except IOError:
                pass

        try:
            os.mkdir('data/')
            print("Directory data created ")
        except FileExistsError:
            print("Directory data exists")

        for track in self.Tracks:
            try:
                # Create target Directory
                os.mkdir('data/'+track)
                print("Directory", track, "created ")
                for name in self.Drivers:
                    try:
                        os.mkdir('data/'+track+'/'+name)
                        print("Directory", name, "created ")
                    except FileExistsError:
                        print("Directory", name, "exists")
 
            except FileExistsError:
                print("Directory", track, "exists")
 
 
    def build_file(self, Driver, Track):
        self.Driver = Driver
        self.Track = Track
        self.writer, self.dictWriter = self.csv_creator()
 
    def delete_file(self):
        if os.path.exists('data/%s/%s/%s_Test_%s.csv' % (self.Track, self.Driver, self.date, self.test_number)):
            os.remove('data/%s/%s/%s_Test_%s.csv' % (self.Track, self.Driver, self.date, self.test_number))
            print('Deleted: data/%s/%s/%s_Test_%s.csv' % (self.Track, self.Driver, self.date, self.test_number))
 
 
    def csv_creator(self):
 
        while os.path.exists('data/%s/%s/%s_Test_%s.csv' % (self.Track, self.Driver, self.date, self.test_number)):
            self.test_number += 1
 
        my_file = open('data/%s/%s/%s_Test_%s.csv' % (self.Track, self.Driver, self.date, self.test_number), "w+")
 
        writer = csv.writer(my_file, delimiter=' ')
        dictWriter = csv.DictWriter(my_file, fieldnames=self.Fields, delimiter=' ', quotechar='|', quoting=csv.QUOTE_MINIMAL)
 
        writer.writerow(['*********************************HEADER*********************************'])
        writer.writerow(['Date:', time.strftime('%y/%m/%d'), ])
        writer.writerow(['Type:', self.Track])
        writer.writerow(['Test:' ,self.test_number])
        writer.writerow(['Driver:', self.Driver])
        writer.writerow(['CANBAUD:', self.BAUD_C])
        writer.writerow(['XBeeBAUD:', self.BAUD_X])
        writer.writerow(['SerialBAUD:', self.BAUD_S])
        # writer.writerow([])
 
 
        writer.writerow(['*********************************HEADER*********************************'])
        dictWriter.writeheader()
        writer.writerow(['START']) #START READING WHERE IT SAYS START
 
 
        print('File Created: data/%s/%s/%s_Test_%s.csv' % (self.Track, self.Driver, self.date, self.test_number))
        return [writer, dictWriter]
 
    def csv_Add(self, data):
        self.writer.writerow(data)
 
    def csv_dictAdd(self, label, data):
        # print("added %s to %s in %s", data, label, self.dictWriter)
        self.dictWriter.writerow({label: data})
