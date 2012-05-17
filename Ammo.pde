

// MAGAZINE function
void magazineupdate(){
  temp = analogRead(magazine_pin);
  if((temp>=25) && (temp<=65) && magazines[0]>0){  // MAG number 0
    magazine_used = 0;
    magazines[0] -= 1;
    barrelbullet = true;
  }
  else if((temp>=285) && (temp<=325) && magazines[1]>0){  // MAG number 1
    magazine_used = 1;
    magazines[1] -= 1;
    barrelbullet = true;
  }
  Serial.println(temp);
}

