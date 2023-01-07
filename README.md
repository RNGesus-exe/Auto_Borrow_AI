# Auto_Borrow_AI

A project written in embedded C using microcontrollers to keep track of borrowed items from a common workplace. This is an open source IOT based project to improve survelliance of tools in work areas. The project can be linked with a mobile app (In my case it I made a Flutter application) and the admins can monitor it to see what item was borrowed by whom.

## Getting Started

Auto_Borrow_AI is meant to be a smart tool rack on which a person can hang his/her items and then leave it on for a day using a cheap rechargable powerbank (whereas in my case I used two rechargable 18650 Lithium ion cell 3.7v). The tools which are used for this IoT based project are listed below:


- Two ESP32 AI Thinker + cam
- Two rechargable 18650 Lithium ion cell 3.7v or a powerbank with atleast 7v
- LM2596 2A Buck Step-down Power Converter Module
- Male to Female, Male to Male and Female to Female Wires (I personally would recommend soldering the wires for the final product)
- Two 18650 lithium ion cell battery holder


<br>The final product will look something like the image below

![product](https://user-images.githubusercontent.com/58368527/211133905-42c5941e-8b37-4e11-bcdf-ddec3a8828c3.jpeg)

## Working

The project works in the following way.

1) On startup the Cameras will do a 3-way handshake to acknowledge that both the cameras are operating properly.

2) After startup, CAM B will first snap a picture of the tool rack to see what items are on the tool rack and then continue to monitor the tool rack for motion to see whether something came into motion or not. If "yes some sort of motion was detected" it will send a signal to CAM A via Firebase by setting a flag in the table to true. This will alert CAM A and it will take a photo of the person/figure infront on it.

3) After motion has stopped occuring in front of CAM B, it will confirm whether an item was borrowed or was it a false alarm. Incase nothing was taken AKA it was a false alarm, CAM B will send a signal to CAM A to discard the picture it took earlier. But if something was taken then CAM B will first check in the EEPROM/SPIFFS to see what item was taken and then upload a cropped image of that item with data-time stamp using the NTP server to Firebase and simultaneuosly telling CAM A to upload the picture it took earlier as well.

4) After uploading the data successfully you can then just write a small script to send a notification on your phone via telegram to update you that an item has been taken. I made an app in Flutter from which I can access the database and see what item was taken by whom.

![labelled_product](https://user-images.githubusercontent.com/58368527/211134614-b2f6ba64-2f48-40af-a209-4e13b60275e5.png)

5) The microcontrollers communicate with each other via a database in the following manner.

![heirarchy](https://user-images.githubusercontent.com/58368527/211135026-172935d5-a562-49a5-b84a-e0012a9bde1f.png)


## Improvements

I fixed most of the issue which I noticed in this project and yes there were alot of ideas I wanted to add in this project, To name a few :

1) Add a machine learning model which can be trained to classify the item and display name just by using the images uploaded on the database.

2) Add a system which can run on AC and charges the powerbank then during a light outage it runs on the powerbank.


## Peronal Remarks (Skip if not here for the monologue)

Do know this is a project I made while studying an IOT course in my university so it does have flaws. I won't be updating or adding new features in this project due to time contraints. The main reason I made this was because in my robotics laboratory the tools kept getting stolen so I decided to make a system which could help reduce these issues in the laboratory. Good news is that I used this system for one month in the laboratory and it has helped and no longer do items get stolen and the lab attendants can keep track of who isssued what item.
