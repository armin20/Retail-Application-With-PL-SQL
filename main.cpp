#include <iostream>
#include <numeric>
#include <occi.h>

using oracle::occi::Environment;
using oracle::occi::Connection;
using namespace oracle::occi;
using std::cout;
using std::endl;
using std::cin;

struct ShoppingCart {
	int product_id;
	double price;
	int quantity;
};

int mainMenu();
int customerLogin(Connection* conn, int customerId);
int addToCart(Connection* conn, struct ShoppingCart cart[]);
double findProduct(Connection* conn, int product_id);
void displayProducts(struct ShoppingCart cart[], int productCount);
int checkout(Connection* conn, struct ShoppingCart cart[], int customerId, int productCount);

int main() {
	Environment* env = nullptr;
	Connection* conn = nullptr;

	std::string user = "dbs311_212za09";
	std::string pass = "31001453";
	std::string constr = "myoracle12c.senecacollege.ca:1521/oracle12c";

	env = Environment::createEnvironment(Environment::DEFAULT);
	conn = env->createConnection(user, pass, constr);
	Statement* stmt = conn->createStatement();
	try
	{

		//stmt->execute("CREATE OR REPLACE PROCEDURE find_customer(p_customer_id IN NUMBER, found OUT NUMBER) IS"
		//	"BEGIN"
		//	"SELECT COUNT(*) INTO found FROM customers"
		//	"WHERE customer_id = p_customer_id;"
		//	"EXCEPTION"
		//	"WHEN NO_DATA_FOUND THEN"
		//	"found := 0;"
		//	"END;");

		//stmt->execute("CREATE OR REPLACE PROCEDURE find_product(p_product_id IN NUMBER, price OUT products.list_price%TYPE) IS"
		//	"p_id products.product_id % TYPE;"
		//	"BEGIN"
		//	"SELECT product_id, list_price INTO p_id, price"
		//	"FROM products"
		//	"WHERE product_id = p_product_id;"
		//	"EXCEPTION"
		//	"WHEN NO_DATA_FOUND THEN"
		//	"price := 0;"
		//	"END;");

		//stmt->execute("CREATE OR REPLACE PROCEDURE add_order(customer_id IN NUMBER, new_order OUT NUMBER) IS "
		//	"BEGIN"
		//	"SELECT MAX(order_id) INTO new_order FROM orders;"
		//	"new_order := new_order + 1;"
		//	"INSERT INTO orders"
		//	"VALUES"
		//	"(new_order, customer_id, 'Shipped', 56, sysdate);"
		//	"END;");

		//stmt->execute("CREATE OR REPLACE PROCEDURE add_order_item(orderId IN order_items.order_id%TYPE,itemId IN order_items.item_id%TYPE, productId IN order_items.product_id%TYPE, o_quantity IN order_items.quantity%TYPE, price IN order_items.unit_price%TYPE) IS "
		//	"BEGIN"
		//	"INSERT INTO order_items"
		//	"VALUES"
		//	"(orderId, itemId, productId, o_quantity, price);"
		//	"END;");


		bool TF = true;
		int select = 0, id = 0, found = 0, productCount = 0;
		ShoppingCart cart[5];

		do
		{
			select = mainMenu();
			switch (select)
			{
			case 1:
				cout << "Enter the customer ID: ";
				cin >> id;
				found = customerLogin(conn, id);
				if (found == 0)
				{
					cout << "The customer does not exist." << endl;
				}
				else
				{
					productCount = addToCart(conn, cart);
					displayProducts(cart, productCount);
					checkout(conn, cart, id, productCount);
				}
				break;
			case 0:

				cout << "Good bye!..." << endl;
				TF = false;
				break;
			}
		} while (TF);
	}
	catch (SQLException& SqlExcp)
	{
		cout << SqlExcp.getErrorCode() << ": " << SqlExcp.getMessage();
	}
	env->terminateConnection(conn);
	Environment::terminateEnvironment(env);

	return 0;
}

int mainMenu() {

	int select = 0;

	cout << "******************** Main Menu ********************\n";
	cout << "1) Login" << endl;
	cout << "0) Exit\n" << endl;
	cout << "Enter an option (0-1): ";
	cin >> select;
	while (select < 0 || select > 1)
	{
		cout << "******************** Main Menu ********************\n";
		cout << "1) Login" << endl;
		cout << "0) Exit\n" << endl;
		cout << "You entered a wrong value. Enter an option (0-1):";
		cin >> select;
	}
	return select;
}

int customerLogin(Connection* conn, int customerId) {

	int found = 0;
	Statement* stmt = conn->createStatement();

	stmt->setSQL("BEGIN"
		" find_customer(:1, :2);"
		" END;"
	);
	stmt->setNumber(1, customerId);
	stmt->registerOutParam(2, Type::OCCIINT, sizeof(found));
	stmt->executeUpdate();
	found = stmt->getInt(2);

	return found;
}

int addToCart(Connection* conn, struct ShoppingCart cart[]) {
	double price = 0.0;
	auto counter = 0, quantity = 0, product_id = 0, select = 0, numProduct = 0;
	bool TF = true;

	cout << "-------------- Add Products to Cart --------------\n";
	do
	{
		cout << "Enter the product ID : ";
		cin >> product_id;
		price = findProduct(conn, product_id);
		if (price != 0)
		{
			cart[counter].product_id = product_id;
			cout << "Product Price : " << price << endl;
			cart[counter].price = price;
			cout << "Enter the product Quantity: ";
			cin >> quantity;
			cart[counter].quantity = quantity;
			counter++;
			numProduct++;
			cout << "\nEnter 1 to add more products or 0 to checkout : ";
			cin >> select;
			(select == 0) ? TF = false : TF = true;
		}
		else
		{
			cout << "The product does not exists. Try again...\n";
		}
	} while (TF);

	return numProduct;
}

double findProduct(Connection* conn, int product_id)
{
	double price = 0.0;
	Statement* stmt = conn->createStatement();

	stmt->setSQL("BEGIN"
		" find_product(:1 , :2);"
		" END;");
	stmt->setNumber(1, product_id);
	stmt->registerOutParam(2, Type::OCCIDOUBLE, sizeof(price));
	stmt->executeUpdate();
	price = stmt->getDouble(2);

	return price;
}
void displayProducts(struct ShoppingCart cart[], int productCount) {

	cout << "------- Ordered Products ---------\n";
	auto i = 0;
	do
	{
		cout << "---Item " << i + 1 << endl
			<< "PRoduct ID: " << cart[i].product_id << endl
			<< "Price: " << cart[i].price << endl
			<< "Quantity: " << cart[i].quantity << endl;
		i++;
	} while (i < productCount);

	auto t = std::accumulate(cart, cart + productCount, double(0), [](double res, ShoppingCart carts) {
		return res + (carts.price * (double)carts.quantity);
		});
	cout << "----------------------------------" << endl
		<< "Total: " << t << endl;

}
int checkout(Connection* conn, struct ShoppingCart cart[], int customerId, int productCount) {

	char ch{};
	bool TF = true;
	do
	{
		cout << "Would you like to checkout? (Y/y or N/n) ";
		cin >> ch;
		if (ch != 'Y' && ch != 'y' && ch != 'n' && ch != 'N')
		{
			cout << "Wrong input. Try again...\n";
		}
		else
		{
			TF = false;
		}
	} while (TF);

	if (ch == 'N' || ch == 'n')
	{
		cout << "The order is cancelled." << endl;
		return 0;
	}

	Statement* stmt = conn->createStatement();
	stmt->setSQL("BEGIN add_order(:1, :2); END;");
	stmt->setNumber(1, customerId);
	stmt->registerOutParam(2, Type::OCCINUMBER);
	stmt->executeUpdate();

	int orderId = stmt->getNumber(2);


	for (auto i = 0; i < productCount; i++)
	{
		stmt->setSQL("BEGIN add_order_item(:1, :2, :3, :4, :5); END;");
		stmt->setNumber(1, orderId);
		stmt->setNumber(2, i + 1);
		stmt->setNumber(3, cart[i].product_id);
		stmt->setNumber(4, cart[i].quantity);
		stmt->setNumber(5, cart[i].quantity);

		stmt->executeUpdate();
	}
	cout << "The order is successfully completed." << endl;
}