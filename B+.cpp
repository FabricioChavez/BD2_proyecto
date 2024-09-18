#include <iostream>
#include <fstream>
#include <tuple>
#include <algorithm>
#include <vector>
#include <string.h>
#include <iomanip>
using  namespace std;
#define m 3
#define page_size  3
template<typename Tk>
struct Record{
    Tk key;
    int code;
    char name[20];


    Record(){

    }

    Record(Tk key, int code, string name_) : key(key), code(code){
        strcpy(name , name_.c_str());
    }
// Constructor de copia
    Record(const Record<Tk>& other) : key(other.key), code(other.code) {
        memcpy(name, other.name, sizeof(name)); // Copia el contenido del arreglo name
    }

    bool operator<(Record<Tk> other) const
    {
        return key < other.key;
    }

    bool operator>(Record<Tk> other) const
    {
        return key > other.key;
    }

    bool operator==(Record<Tk> other) const
    {
        return key == other.key;
    }



};//24 size
template<typename Tk>
std::ostream& operator<<(std::ostream& os, const Record<Tk>& record) {
    os << "Key: " << record.key << ", Code: " << record.code << ", Name: " << record.name;
    return os;
}


template<typename Tk>
struct indexNode{
    Tk key[m-1];
    int children[m];
    int count;
    int next_removed;
    bool is_last;
    bool is_root;

    indexNode(){

        next_removed=-1;
        count=0;
        is_last= false;
        is_root = false;
    }


    void showdata()
    {
        cout<<"-------------Index data-------------"<<endl;
        cout<<"Root status  :"<<boolalpha<<is_root<<endl;
        cout<<"Last status  :"<<boolalpha<<is_last<<endl;
        cout<<"Count        :"<<count<<endl;
        cout<<"Next removed :"<<next_removed<<endl;
        cout<<">>>>>>>>Keys >>>>>>>>>>>>>>>>>>>>>>"<<endl;
        for (int i = 0; i <count ; ++i) {
            cout<<setw(10)<<" ["<<key[i]<<" ]\t";
        }
        cout<<">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>"<<endl;
        cout<<"<<<<<<<<Children pointer<<<<<<<<<<<"<<endl;

        if(count>0)
        {
            for (int i = 0; i <count+1 ; ++i) {
                cout<<setw(10)<<"[ "<<children[i]<<" ]\t ";
            }
        }
        cout<<endl;

        cout<<"<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<"<<endl;


    }

};
//sizeof(Tk)*(2m-1) + 8
template<typename Tk>
struct dataPage{
    Record<Tk> records[page_size];
    int next_page;
    int next_removed;
    int count;

    dataPage(){
        next_page=-1;
        next_removed=-1;
        count=0;
    }

    void showdata()
    {
        cout<<"-------------Page data-------------"<<endl;
        cout<<"next_page      :"<<next_page<<endl;
        cout<<"next_removed   :"<<next_removed<<endl;
        cout<<"Count          :"<<count<<endl;
        cout<<"<<<<<<<<RECORDS<<<<<<<<<<<"<<endl;
        for (int i = 0; i <count ; ++i) {
            cout<<records[i]<<endl;
        }
        cout<<"<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<"<<endl;


    }

};


string indexfile = "index.bin";
string datafile = "data.bin";

template<typename Tk>
class BplusTree{
private:
    indexNode<Tk> * root;
    dataPage<Tk> current;
    int metadata_size = int(sizeof(int));
    int index_length;
    int data_length;
    int index_fl_top; //Cabeza de puntero logico de la free list en indexfile
    int data_fl_top; //Cabeza de puntero logico de la free list en datafile
public:

    BplusTree(){
        index_length = sizeof(indexNode<Tk>);
        data_length = sizeof(dataPage<Tk>);
        create_index_files();//Create the new files in case they do not exist and write the metadata for freelist
       create_page_file();
        lift_metadata(); // Lift metadata if possible
    }


    void insert(Record<Tk> record )
    {

        fstream file_index(indexfile , ios::binary | ios::in|ios::out);
        fstream file_data(datafile , ios::binary |ios::in | ios::out);
        int r_default = -1;
        int l_default = -1;
        Tk key_default;
        bool flag = false;
        insert(file_index , file_data , record , 0 , l_default ,r_default ,key_default , flag );

        file_index.close();
        file_data.close();



    }


    void insert(fstream& file_index , fstream  & file_data, Record<Tk>& record,int logical_pointer , int &left_up , int& right_up , Tk &up_key , bool &there_is_excess  )
    {
        indexNode<Tk> node = read_index_node_in_pos(file_index , logical_pointer);
        if(node.is_last)
        {

            if(node.count == 0) // Si realmente aun no hay nada en root solo se inserta directamente
            {
//                cout<<"Insertando primera vez :3"<<endl;
                if(insert_in_leaf(file_data ,0 , record)) {
                    return;
                }else {
//                    cout<<"Primer split"<<endl;
                    //Primer split
                    dataPage<Tk> b = read_page_node_in_pos(file_data , 0);
                    tuple<int , int , Tk> lrk = split_leaf_node(file_data , 0, b , record  );
                    //Dos nuevos primeros nodos generados
                    int left_leaf_pointer = get<0>(lrk);
                    int right_leaf_pointer =get<1>(lrk);
                    Tk split_key = get<2>(lrk);

                    //Insertar ordenadamente , Al ser la primera vez no se tienen en cuenta un posible spliteo
                    insert_in_index(file_index , 0 , split_key , left_leaf_pointer  , right_leaf_pointer);
                    return;
                }

            }


            int pos =  int(upper_bound(node.key  , node.key + node.count , record.key) - &node.key[0]) ;

//            cout<<"Se ubicara en la siguiente posicion "<<node.children[pos]<<"para insertar "<<record.key<<endl;

            if(insert_in_leaf(file_data , node.children[pos], record)) {
                return;
            }else {
                //Si no hay espacio se splitea la hoja y luego se inserta esto en el index
                dataPage<Tk> b = read_page_node_in_pos(file_data , node.children[pos]);
                tuple<int , int , Tk> lrk = split_leaf_node(file_data , node.children[pos], b , record  );
                //Dos nuevos primeros nodos generados
                int left_leaf_pointer = get<0>(lrk);
                int right_leaf_pointer =get<1>(lrk);
                Tk split_key = get<2>(lrk);
//                cout<<"LEFT LEAF  "<<left_leaf_pointer<<endl;
//                cout<<"RIGHT LEAF "<<right_leaf_pointer<<endl;
//                cout<<"NEW KEY    "<<split_key<<endl;


                //insertar ordenadamente la nueva key y ubicar a sus hijos
                if(insert_in_index(file_index , logical_pointer , split_key , left_leaf_pointer , right_leaf_pointer))
                {
                    //si se inserta si problemas en el nodo solo retornas tranquilamente
                    return;
                }else {
                    //Si este nodo es la raiz y a la vez sabemos que es el last entones es la primera vez q sube esta propiedad de raiz y hace crecer el arbol
                    if(node.is_root)
                    {
//                        cout<<"VAMOS A SPLITEAR NUESTRA PRIMERA ROOT :3"<<endl;
                        indexNode<Tk> new_root;
                        new_root.is_root =true;
                        new_root.is_last = false;
                        int left_index ;
                        int right_index;
                        Tk new_root_key ;
                        //splitear el nuevo index (las hojas se asignaran dentro ordenadamente a cada mitad correspondiente)
                       split_index_node(file_index ,logical_pointer,node , split_key , left_leaf_pointer , right_leaf_pointer , left_index , right_index  , new_root_key);



//                        cout<<"ESO ES LO QUE SALE DE SPLITEAR LA ROOT EN LA POSICIONDADA "<<pos<<"CON ESTOS DATOS GENERADOS"<<endl;
//                        cout<<"LEFT INDEX  "<<left_up<<endl;
//                        cout<<"RIGHT INDEX "<<right_index<<endl;
//                        cout<<"NEW KEY ROOT   "<<new_root_key<<endl;

                        //calculuar un nuevo index logico este sera el nuevo left
                        int new_logical_index = calculate_index(node);
//                        cout<<"La nueva posicion logica para la raiz es "<<new_logical_index<<endl;
                        //Extraer el puntero izquierdo de su posicion
                        indexNode<Tk> old_left = read_index_node_in_pos(file_index , left_index);
                        //escribir left en la nueva posicion
                        write_index_node_in_pos(file_index,old_left , new_logical_index);
                        //escribir  root en la poscion cero
                        new_root.key[new_root.count]=new_root_key; //asigna la primera key del split
                        new_root.children[new_root.count]=new_logical_index;//se asigna nodo de la izquierda
                        new_root.children[++new_root.count]=right_index;//se asigna nodo de la derehca

                        //escribir la raiz --> siempre posicion cero
                        write_index_node_in_pos(file_index , new_root, 0);
                        //retornar
                        return;

                    }


//                    // En caso de que no sea root solo se splitea y se manda hacia arriba los valores a insertar en el nivel superior (todo lo demas sera manegada por la vuelta de la recursion)
//                    cout<<"INDEX --->IZQUIERDA QUE SURGE antes  "<<left_up<<endl;
//                    cout<<"INDEX --->DERECHA QUE SURGE antes  "<<right_up<<endl;
//                    cout<<"INDEX --->KEY QUE SURGE antes "<<up_key<<endl;
                    //splitear el nuevo index (las hojas se asignaran dentro ordenadamente a cada mitad correspondiente)
                    split_index_node(file_index, logical_pointer, node, split_key, left_leaf_pointer, right_leaf_pointer , left_up , right_up , up_key);

//                    cout << "Valores devueltos por split_index_node:\n";
//                    cout << "left_index: " << left_up << "\n";
//                    cout << "right_index: " << right_up << "\n";
//                    cout << "new_root_key: " << up_key << "\n";



                    //Con esto se indica a los niveles superiores en la vuelta de la recursion que existe algo que insertar
                    there_is_excess= true;
                    //Retornarn
                    return;

                }


            }


        }else{
                //next index te da el indice logico q esta en el child no se use para obtener donde insertar algo

            int next_index = get_next_index(node.key ,  node.count, record.key  , node.children );
//            cout<<"AUN NO ESTOY EN ROOT PERO A VER Q PASA  mi next pos es la SIGUIENTE"<<next_index<<endl;
            insert(file_index , file_data , record , next_index , left_up , right_up , up_key  , there_is_excess);

        }


        //Vuelta de la recursion donde los milagros pasan ;3
//        cout<<"OK VAMOS DE VUELTA ..."<<endl;


        if(there_is_excess)
        {

            //Si no es root no se hace nada fundamentalmetne diferente , ya que se asume que suben los valores
            if(insert_in_index(file_index ,  logical_pointer , up_key , left_up , right_up ))
            {
//                cout<<"SE INSERTO EL EXCESO ACA"<<endl;
                //si se inserta y no hay problema solo se pone este flag en falso y ya
                there_is_excess= false;
            }else{
                int l = left_up;
                int r = right_up;
                Tk k = up_key;

                //Si el nodo que se tiene que splitear es root se diverge un poco
                if(node.is_root){

                    indexNode<Tk> new_root;
                    new_root.is_root =true;
                    new_root.is_last = false;
                    int left_index ;
                    int right_index;
                    Tk new_root_key ;
                    //splitear el nuevo index (las hojas se asignaran dentro ordenadamente a cada mitad correspondiente)
                    split_index_node(file_index ,logical_pointer,node , k , l , r , left_index , right_index  , new_root_key);


                    //calculuar un nuevo index logico este sera el nuevo left
                    int new_logical_index = calculate_index(node);
                    //Extraer el puntero izquierdo de su posicion
                    indexNode<Tk> old_left = read_index_node_in_pos(file_index , left_index);
                    //escribir left en la nueva posicion
                    write_index_node_in_pos(file_index,old_left , new_logical_index);
                    //escribir  root en la poscion cero
                    new_root.key[new_root.count]=new_root_key; //asigna la primera key del split
                    new_root.children[new_root.count]=new_logical_index;//se asigna nodo de la izquierda
                    new_root.children[++new_root.count]=right_index;//se asigna nodo de la derehca

                    //escribir la raiz --> siempre posicion cero
                    write_index_node_in_pos(file_index , new_root, 0);
                    there_is_excess = false;

                }else {

                    //splitear el nuevo index (las hojas se asignaran dentro ordenadamente a cada mitad correspondiente)
                    split_index_node(file_index ,logical_pointer,node , k , l , r    ,left_up , right_up , up_key);

                    //Con esto se indica a los niveles superiores en la vuelta de la recursion que existe algo que insertar
                    there_is_excess= true;
                    //Retornarn

                }

            }
        }




    }




    Record<Tk>* search(Tk key)
    {
        fstream file_index(indexfile , ios::binary | ios::in|ios::out);
        fstream file_data(datafile , ios::binary |ios::in | ios::out);

        Record<Tk>* ans= search(key , 0 , file_index , file_data);
        file_index.close();
        file_data.close();
        return ans;
    }

    Record<Tk> * search(Tk key , int current_node  , fstream & file_index , fstream & file_data)
    {
            indexNode<Tk> node = read_index_node_in_pos(file_index , current_node);

            if(node.is_last)
            {

                int possible_pos = int(upper_bound(node.key , node.key +node.count  , key)-&node.key[0]);

                dataPage<Tk> search_page = read_page_node_in_pos(file_data , node.children[possible_pos]);
                Record<Tk>* ans = search_in_page(search_page , key);

                return ans;

            }


        int next_index = get_next_index(node.key ,  node.count, key , node.children );

       return search(key , next_index , file_index , file_data);



    }

    vector<Record<Tk>> range_search(Tk beg , Tk fin ){

    }



    void remove(Tk key){

    }


    void print_all(){

        indexNode<Tk> index_node;
        dataPage<Tk> data_node;

        fstream file_index(indexfile , ios::binary | ios::in|ios::out);
        fstream file_data(datafile , ios::binary |ios::in | ios::out);
        int total_pages = get_total_size<dataPage<Tk>>(datafile);
        int index_total = get_total_size<indexNode<Tk>>(indexfile);

        cout<<"Number of indexes -->"<<index_total<<endl;
        cout<<"Number of pages ---->"<<total_pages<<endl;

        cout<<"-------------INDEX NODES BEGIN ---------------------------"<<endl;
        for (int i = 0; i < index_total; ++i) {
            index_node = read_index_node_in_pos(file_index , i);
            cout<<"-------index Node number ["<<i<<"]"<<"-----------"<<endl;
            index_node.showdata();
            cout<<"---------------------------------------------"<<endl;

        }

        cout<<"-------------INDEX NODES END---------------------------"<<endl;

        cout<<"-------------DATA NODES BEGIN ---------------------------"<<endl;
        for (int i = 0; i < total_pages; ++i) {
            data_node = read_page_node_in_pos(file_data , i);
            cout<<"------- data Node number ["<<i<<"]"<<"-----------"<<endl;
            data_node.showdata();
            cout<<"---------------------------------------------"<<endl;

        }
        cout<<"-------------DATA NODES END -----------------------------"<<endl;



    }


private:
    //funcion para buscar al elemento
        Record<Tk>* search_in_page(dataPage<Tk>& page, Tk key)
        {
            int left = 0;
            int right = page.count -1;

            while (left <= right) {
                int mid = left + (right - left) / 2;

                // Comparar el key con el elemento en el medio
                if (page.records[mid].key == key) {
                    return new Record<Tk>(page.records[mid]); // Devuelve un puntero al Record encontrado
                } else if (page.records[mid].key < key) {
                    left = mid + 1;
                } else {
                    right = mid - 1;
                }
            }
            return nullptr; // No se encontró el Record
        }

    //funciones de insercion
    //inserta ordenadamente en hoja
    bool insert_in_leaf(fstream & file, int index  , const Record<Tk>& record){


       dataPage<Tk> to_insert = read_page_node_in_pos(file , index);

        if(to_insert.count+1>page_size) {
//            cout<<"la proxima insercion romple reglas"<<endl;
            return false;
        }



        if(to_insert.count ==0){

            to_insert.records[to_insert.count++]=record;
            write_page_node_in_pos(file , to_insert , index);
            return true;
        }
        //encuentra la poscion del primer elemento mayor al que se inserta
       int  pos  = upper_bound(to_insert.records , to_insert.records+to_insert.count , record) - &to_insert.records[0];
        //inserta ordenadamente en la posicon
        for (int i = ++to_insert.count-1; i >=pos ; --i) {\
            if(i==pos)
                to_insert.records[i]=record;
            if(i>pos)
                to_insert.records[i]=to_insert.records[i-1];
        }

        //Escribir el valor del bucker actualizado en memoria
        write_page_node_in_pos(file , to_insert , index); //
        //retorna true una vez completado el proceso
        return true;

    }

    //inserta ordenadamente en nodo interno
    bool insert_in_index(fstream & file , int index, Tk new_key, int left_pointer, int right_pointer) {

        indexNode<Tk> to_insert = read_index_node_in_pos(file, index);



        // Si se llena con esto se mandaría a un split, por lo tanto retornamos falso
        if(to_insert.count + 1 > m - 1) return false;

        // Si es el primer insert, solo se pone en la primera posición
        if(to_insert.count == 0) {
            to_insert.children[0] = left_pointer;
            to_insert.children[1] = right_pointer;
            to_insert.key[0] = new_key;
            to_insert.count++;
            write_index_node_in_pos(file, to_insert, index);
            return true;
        }

        // Encuentra la posición adecuada de la key
        int pos = int(upper_bound(to_insert.key, to_insert.key + to_insert.count, new_key) - &to_insert.key[0]);

//        cout << "BEFORE THE INSERT" << endl;
//        to_insert.showdata();

        // Insertamos la nueva clave y ajustamos punteros en un solo ciclo
        for (int i = to_insert.count; i >= pos; --i) {
            // Mover las claves y punteros hacia adelante si es necesario
            if (i > pos) {
                to_insert.key[i] = to_insert.key[i - 1];
                to_insert.children[i + 1] = to_insert.children[i];
            }
            // Insertar la nueva clave y los punteros correspondientes
            if (i == pos) {
                to_insert.key[i] = new_key;
                to_insert.children[pos] = left_pointer;
                to_insert.children[pos + 1] = right_pointer;
            }
        }

        // Incrementar el conteo de claves en el nodo
        to_insert.count++;
//
//        cout << "AFTER THE INSERT" << endl;
//        to_insert.showdata();

        // Escribimos el nodo actualizado
        write_index_node_in_pos(file, to_insert, index);

        // Retornamos verdadero una vez concluye el proceso
        return true;
    }




    // Retorna left , right , key
    tuple<int, int , Tk> split_leaf_node( fstream &file ,int index,dataPage<Tk> & to_split  , Record<Tk> new_record   ){

        Record<Tk> r[page_size+1];
        dataPage<Tk> left_old;
        dataPage<Tk> right_new;
        Tk new_key;
        int mid = (page_size+1)/2;

        //calcular posicion donde debe introducirce la key ficticia para hacer el split
        auto pos  = upper_bound(to_split.records , to_split.records+page_size , new_record) - &to_split.records[0];

        //Pasar todos los nodos a un lugar con un lugar mas para posicionarlos
        for (int i = 0; i < page_size +1 ; ++i) {

            if(i<pos)
                r[i]=to_split.records[i];


            if(i==pos)
                r[i]=new_record;

            if(i>pos)
                r[i]=to_split.records[i-1];

            if(i==mid) new_key =  r[i].key ;

        }

        //Hacer el split de los nodos
        for (int i = 0; i < page_size+1; ++i) {

            if(i< mid)
                left_old.records[left_old.count++] = r[i];
            if(i>=mid)
                right_new.records[right_new.count++] =r[i];
        }
        dataPage<Tk> dummy;//variable para activar sobrecarga
        int next_logical_index = calculate_index(dummy);  //calcuar pos para ubicar el nuevo page bucket generado

        cout<<"ESTA ES LA SIGUIENTE POSICION LOGICA -->"<<next_logical_index<<endl;
        right_new.next_page = to_split.next_page; // Asignar puntero a la derecha que el nodo original apuntaba
        left_old.next_page =next_logical_index;  //  asignar el valor del index nuevo a la hoja izquierda para enlazarlos
        write_page_node_in_pos(file , left_old , index); // escribir el left en la posicon original
        write_page_node_in_pos(file , right_new , next_logical_index); // escribir el right en la nueva posicion

        tuple<int , int , Tk> lrk = {index , next_logical_index , new_key};

        return lrk;

    }

    void  split_index_node(fstream &file ,int index,indexNode<Tk> & to_split  , Tk key  , int new_child_l , int new_child_right  , int & l_ans , int & r_ans , Tk& ans){
        Tk rkey[m];
        int rchildren[m+1];
        indexNode<Tk> left;
        indexNode<Tk> right;
        left.is_last = right.is_last = to_split.is_last; // Si se da el caso de que el nodo es lasts se pasa esa propiedad si no lo es no se pasa la propiedad

        Tk new_key;
        int mid = (m)/2;
        auto pos = upper_bound(to_split.key , to_split.key + to_split.count , key)  - &to_split.key[0];

        //Reorganizar nodos internos
        for (int i = 0; i < m+1; ++i) {

            if(i < pos )
            {
                if(i<m)
                rkey[i]=to_split.key[i];

                rchildren[i]=to_split.children[i];
            }

            if(i == pos){
                if(i<m)
                    rkey[i]=key;

                rchildren[i]= new_child_l;
                rchildren[i+1]= new_child_right;
            }


            if(i > pos)
            {

                if(i<m)
                    rkey[i] = to_split.key[i-1];

                if(i>pos+1)
                rchildren[i]=to_split.children[i-1];
            }


            if(i==mid) new_key = rkey[i];
        }

        cout<<"KEYS"<<endl;
        for (int j = 0; j < m; ++j) {
            cout<<rkey[j]<<" ";
        }
        cout<<endl;
        cout<<"CHILDREN"<<endl;
        for (int j = 0; j < m+1; ++j) {
            cout<<rchildren[j]<<" ";
        }

        //Splitear nodo interno
        int i=0;
        for ( ; i < mid ; ++i) {
            left.children[left.count]=rchildren[i];
            left.key[left.count++]=rkey[i];

        }

        left.children[left.count] = rchildren[i];

        for(++i; i<m ; ++i)
        {    right.children[right.count]=rchildren[i];
            right.key[right.count++]=rkey[i];
        }

        right.children[right.count]=rchildren[i];

        //fin de split

        cout<<"LEFT"<<endl;
        left.showdata();
        cout<<"RIGHT"<<endl;
        right.showdata();


        indexNode<Tk> dummy;
        int next_logical_index = calculate_index(dummy);
        cout<<" SE VAN A ESCRIBIR EN LAS SIQUIENTES POSCIONES "<<index<< " Y "<<next_logical_index<<endl;
        write_index_node_in_pos(file , left , index);
        write_index_node_in_pos(file , right , next_logical_index);


        l_ans = index;
        cout<<"EL VALOR ACA DE MI ANSWER ES ESTA"<<l_ans<<endl;
        r_ans = next_logical_index;
        cout<<"EL VALOR ACA DE MI ANSWER ES ESTA AHORA "<<l_ans<<endl;
        ans = new_key;




    }
    //obtiene el puntero en child para la recursion
    int get_next_index(Tk  keys[m-1], int size, Tk key ,  int children[m] ){
       int pos =   int(upper_bound(keys  , keys + size , key) - &keys[0]) ;
        return children[pos];

    }

    int calculate_index(dataPage<Tk> ){ //calculate next logical index or assign you a free space from the free list
        fstream file(datafile , ios::in| ios::out);
        dataPage<Tk> b ;

        if(data_fl_top != -1)
        {   int ans = data_fl_top;
            b = read_page_node_in_pos(file , data_fl_top );
            data_fl_top = b.next_removed;
            file.seekp(0,ios::beg);
            file.write((char*)(&data_fl_top), metadata_size);
            return ans;
        }

        file.seekg(0 , ios::end);
        int total = (int(file.tellg())-metadata_size)/data_length;
        file.close();
        return total;

    }



    int calculate_index(indexNode<Tk>) {
        fstream file(indexfile, ios::in|ios::out);
        indexNode<Tk> b;

        if (index_fl_top != -1) {
            int ans = index_fl_top;  // Cambiado de data_fl_top a index_fl_top
            b = read_index_node_in_pos(file, index_fl_top);
            index_fl_top = b.next_removed;
            file.seekp(0, ios::beg);
            file.write((char*)(&index_fl_top), metadata_size);
            file.flush();  // Asegura que la actualización del free list se refleje en disco
            return ans;
        }

        file.seekg(0, ios::end);
        int total = (int(file.tellg()) - metadata_size) / index_length;
        file.close();
        return total;
    }



    template<class T>
    void add_bucket_to_end(const string& filename   )
    {
        T generic_bucket;
        fstream file(filename , ios::binary | ios::in | ios::out);
        file.seekp(0 , ios::end);
        file.write((char*)(&generic_bucket) , sizeof(T));
    }


    template<class T>
    int get_total_size(string filename){

        fstream file(filename ,ios::binary|ios::in|ios::out);
        file.seekg(0 , ios::end);
        cout<<"GET TOTAL SIZE"<<file.tellg()<<endl;
        int total = int(file.tellg()) - metadata_size;
        int object_size = sizeof(T);
        file.close();
        return total/object_size;
    }

    void create_page_file(){
        //Crear indexfile si no existiera
        dataPage<Tk> default_page_node;
        fstream file(datafile , ios::binary|ios::in|ios::out);

        if(!file.is_open())
        {
            file.open(datafile, ios::binary|ios::out);
            file.close();


            int default_free_list_top = -1;
            file.open(datafile , ios::binary|ios::in|ios::out);
            file.seekp(0, ios::beg);
            file.write((char*)(&default_free_list_top) , metadata_size);
            file.write((char*)(&default_page_node) ,data_length);
            file.seekg(0,ios::end);
            cout<<file.tellg()<<endl;
            file.close();
        }

    }

    void create_index_files(){

        indexNode<Tk> default_node;
        default_node.is_root=true;
        default_node.is_last = true;

        //Crear datafile si no existiera
        fstream file(indexfile , ios::binary|ios::in|ios::out);

        if(!file.is_open())
        {
            file.open(indexfile, ios::binary|ios::out);
            file.close();


            int default_free_list_top = -1;
            file.open(indexfile , ios::binary|ios::in|ios::out);
            file.seekp(0, ios::beg);
            file.write((char*)(&default_free_list_top) , metadata_size);
            file.write((char*)(&default_node) , index_length); //inicializa la root

            file.seekg(0,ios::end);
            cout<<file.tellg()<<endl;

            file.close();

        }


    }
    void lift_metadata(){
        //Leer metadata de datafile
        fstream file(datafile , ios::binary|ios::in|ios::out);
        file.seekg(0 , ios::beg);
        file.read((char*)(&data_fl_top) , metadata_size);
        file.close();
        //Leer medatada de indexfile
        fstream file2(indexfile , ios::binary|ios::in|ios::out);
        file2.seekg(0 , ios::beg);
        file2.read((char*)(&index_fl_top) , metadata_size);
        file2.close();

    }


    indexNode<Tk> read_index_node_in_pos(fstream& file, int pos)
    {
        indexNode<Tk> index_node;
        file.seekg(metadata_size+ pos*index_length, ios::beg);
        file.read((char*)(&index_node) , index_length);

        return index_node;
    }
    void write_index_node_in_pos(fstream& file ,const indexNode<Tk> index_node , int pos){
        file.seekp(metadata_size + pos*index_length , ios::beg);
        file.write((char*)(&index_node) , index_length);
        file.flush();  // Asegura que los datos se escriban en disco
    }


    dataPage<Tk> read_page_node_in_pos(fstream &file  , int pos){
        dataPage<Tk> data_node;
        file.seekg(metadata_size+ pos*data_length, ios::beg);
        file.read((char*)(&data_node) , data_length);
        return data_node;

    }
    void write_page_node_in_pos(fstream& file ,dataPage<Tk>& data_node , int pos){
        file.seekg(metadata_size+ pos*data_length, ios::beg);
        file.write((char*)(&data_node) , data_length);

    }




};
// Función que genera y devuelve un vector de records
template<typename Tk>
std::vector<Record<Tk>> generateRecords() {
    // Inicializar generador de números aleatorios
    std::srand(std::time(0));

    // Lista de keys
    Tk keys[] = {27, 16, 24, 10, 37, 15, 25, 38, 32, 8, 31, 2, 45, 6, 36 , 50 , 33 , 34,28,29};
    int num_keys = sizeof(keys) / sizeof(keys[0]);

    // Lista de nombres de personajes (Cinderella Girls Idol m@sterU149, Madoka Magica, Bakemonogatari)
    const char* names[] = {
            "Chika Yokoyama",      // Cinderella Girls Idol m@sterU149
            "Mai Fukuyama",        // Cinderella Girls Idol m@sterU149
            "Kozue Yusa",          // Cinderella Girls Idol m@sterU149
            "Koharu Koga",         // Cinderella Girls Idol m@sterU149
            "Momoka Sakurai",      // Cinderella Girls Idol m@sterU149
            "Yukimi Sajo",         // Cinderella Girls Idol m@sterU149
            "Chie Sasaki",         // Cinderella Girls Idol m@sterU149
            "Arisu Tachibana",     // Cinderella Girls Idol m@sterU149
            "Haru Yuuki",          // Cinderella Girls Idol m@sterU149
            "Madoka Kaname",       // Madoka Magica
            "Homura Akemi",        // Madoka Magica
            "Mami Tomoe",          // Madoka Magica
            "Hitagi Senjougahara", // Bakemonogatari
            "Mayoi Hachikuji",     // Bakemonogatari
            "Tsubasa Hanekawa",    // Bakemonogatari
            "Remilia Scarlet",     // Touhou Embodiment of The Scarlet Devil ;3 (me)
            "Sakuya Izayoi"  ,      // AKA JACK the ripper from Touhou Embodiment of The Scarlet Devil ;3 (me)
            "Mima the forgotten",   // PC 98 TH
            "Renko Usami ",         // Retrospective 53 minutes
            "Maribel Hearn"         //Retrospective 53 minutes
    };

    // Crear vector para almacenar los registros
    std::vector<Record<Tk>> records;

    for (int i = 0; i < num_keys; ++i) {
        Record<Tk> rec;
        rec.key = keys[i];
        rec.code = std::rand() % 9000 + 1000;  // Generar código aleatorio de 4 dígitos
        strncpy(rec.name, names[i], sizeof(rec.name) - 1);  // Asignar nombre correspondiente
        rec.name[sizeof(rec.name) - 1] = '\0';  // Asegurar que la cadena esté terminada en null

        records.push_back(rec);
    }

    return records;  // Devolver el vector de records
}

int main(){

   vector<Record<int>> r = generateRecords<int>();

   BplusTree<int> test;
//
//    for (int i = 0; i < r.size() ; ++i) {
//        cout<<"INSERTANDO..."<<r[i].key<<endl;
//        test.insert(r[i]);
//    }
cout<<"_________________________________SEARCH TESTS ____________________________________"<<endl;
//test.print_all();
vector<int> search_key{28,29,50,33 , 100};


for(int  vals:search_key)
{
    auto *it = test.search(vals);
    if(it== nullptr)
        cout<<vals<<" not found..."<<endl;
    else cout<<*it<<endl;
}




}